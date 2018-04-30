#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhaseStxxl.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>

namespace tdc { namespace lcpcomp {

constexpr uint64_t numberOfVectors = 5;
constexpr double memoryPercentageVectorCache = .3125;

constexpr uint64_t blockSize = 2 * 1024 * 1024; // 2MiB per block
constexpr uint64_t pageSize = 4;                // 4 blocks per page
constexpr uint64_t cachedPages = 8;             // 8 cached pages (will be adjusted later)
constexpr uint64_t minBytesMemoryPerVector = cachedPages * pageSize * blockSize;
constexpr uint64_t minBytesMemory = len_t((minBytesMemoryPerVector * numberOfVectors) / memoryPercentageVectorCache);

template <typename unsigned_t>
struct literal_t {
 public:
    unsigned_t copyTo;
    char character;
    literal_t() {}        
    literal_t(unsigned_t copyTo, char character) {
        this->copyTo = copyTo;
        this->character = character;
    }
};

template <typename unsigned_t>
struct factor_t {
 public:
    unsigned_t copyFrom, length;
    factor_t() {}        
    factor_t(unsigned_t copyFrom, unsigned_t length) {
        this->copyFrom = copyFrom;
        this->length = length;
    }
    
    unsigned_t copyFromEnd() const {
        return copyFrom + length;
    }
};

template <typename unsigned_t>
struct reference_t : public factor_t<unsigned_t> {
    unsigned_t copyTo;
    reference_t() {}        
    reference_t(unsigned_t copyTo, unsigned_t copyFrom, unsigned_t length) 
            : factor_t<unsigned_t>(copyFrom, length) {
        this->copyTo = copyTo;
    }
    void removePrefix(unsigned_t prefixLength) {
        copyTo += prefixLength;
        factor_t<unsigned_t>::copyFrom += prefixLength;
        factor_t<unsigned_t>::length -= prefixLength;
    }
    
    unsigned_t copyToEnd() const {
        return copyTo + factor_t<unsigned_t>::length;
    }
    
    template<typename unsigned_other_t>
    reference_t<unsigned_other_t> convert() {
        return reference_t<unsigned_other_t>(
            uint64_t(copyTo), 
            uint64_t(factor_t<unsigned_t>::copyFrom), 
            uint64_t(factor_t<unsigned_t>::length));
    }
};


typedef uint_t<40> unsigned_initial_t;
typedef reference_t<unsigned_initial_t> ref_initial_t;
typedef typename stxxl::VECTOR_GENERATOR<ref_initial_t, pageSize, cachedPages, blockSize>::result vector_of_ref_initial_t;
typedef typename vector_of_ref_initial_t::bufwriter_type vector_of_ref_writer_initial_t;
typedef typename vector_of_ref_initial_t::bufreader_type vector_of_ref_reader_initial_t;
typedef typename stxxl::VECTOR_GENERATOR<char, pageSize, cachedPages, blockSize>::result vector_of_char_t;
typedef typename vector_of_char_t::bufwriter_type vector_of_char_writer_t;
typedef typename vector_of_char_t::bufreader_type vector_of_char_reader_t;


template <unsigned bitsPerUInt>
class PLCPDecomp {
    friend class PLCPDecompGenerator;
 public:
    
    typedef uint_t<bitsPerUInt> unsigned_t;    
    typedef reference_t<unsigned_t> ref_t;
    typedef literal_t<unsigned_t> lit_t;
    typedef typename stxxl::VECTOR_GENERATOR<ref_t, pageSize, cachedPages, blockSize>::result vector_of_ref_t;
    typedef typename vector_of_ref_t::bufwriter_type vector_of_ref_writer_t;
    typedef typename vector_of_ref_t::bufreader_type vector_of_ref_reader_t;
    typedef typename stxxl::VECTOR_GENERATOR<lit_t, pageSize, cachedPages, blockSize>::result vector_of_lit_t;
    typedef typename vector_of_lit_t::bufwriter_type vector_of_lit_writer_t;
    typedef typename vector_of_lit_t::bufreader_type vector_of_lit_reader_t;
    
    typedef typename stxxl::VECTOR_GENERATOR<char, pageSize, cachedPages, blockSize>::result::size_type vector_size_t;
    
 private:

    struct reference_limits_t {
        const unsigned_t min_unsigned = std::numeric_limits<unsigned_t>::min();
        const unsigned_t max_unsigned = std::numeric_limits<unsigned_t>::max();
        const ref_t min_factor = ref_t(min_unsigned, min_unsigned, min_unsigned);
        const ref_t max_factor = ref_t(max_unsigned, max_unsigned, max_unsigned);
        ref_t min_value() const { return reference_limits_t::min_factor; }
        ref_t max_value() const { return reference_limits_t::max_factor; }
    };

    struct compare_by_copyTo_t : reference_limits_t {
        bool operator()(const ref_t &a, const ref_t &b) const 
        { return a.copyTo < b.copyTo; }
    } compare_by_copyTo;
    
    struct compare_by_copyFrom_t : reference_limits_t {
        bool operator()(const ref_t &a, const ref_t &b) const 
        { return a.copyFrom < b.copyFrom; }
    } compare_by_copyFrom;
    
    struct literal_limits_t {
        const unsigned_t min_unsigned = std::numeric_limits<unsigned_t>::min();
        const unsigned_t max_unsigned = std::numeric_limits<unsigned_t>::max();
        const lit_t min_literal = lit_t(min_unsigned, '\0');
        const lit_t max_literal = lit_t(max_unsigned, '\0');
        lit_t min_value() const { return literal_limits_t::min_literal; }
        lit_t max_value() const { return literal_limits_t::max_literal; }
    };
    
    struct compare_by_copyTo_lit_t : literal_limits_t {
        bool operator()(const lit_t &a, const lit_t &b) const 
        { return a.copyTo < b.copyTo; }
    } compare_by_copyTo_lit;
    
    vector_of_char_t &textBuffer;
    
    vector_of_ref_t * byCopyToV;
    vector_of_ref_t * byCopyToV_new;
    vector_of_ref_writer_t * byCopyToV_writer;
    
    vector_of_ref_t resolvedV;
    vector_of_ref_writer_t * resolvedV_writer;
    
    uint64_t sortBytesMemory;
    uint64_t cachedPagesPerVector;
    
    ~PLCPDecomp() {
        delete byCopyToV;
        delete byCopyToV_new;
        delete byCopyToV_writer;
        delete resolvedV_writer;
    }
    
    void scan();
    bool resolve(ref_t &fromFactor, const ref_t &toFactor);
    void restore();
    
    static uint64_t getNumberOfCachedPages(uint64_t totalBytesMemory) {
        totalBytesMemory = std::max(totalBytesMemory, minBytesMemory);
        uint64_t bytesPerVector = uint64_t((totalBytesMemory * memoryPercentageVectorCache) / numberOfVectors);
        return (bytesPerVector / blockSize) / pageSize;
    }

 public:
    
    PLCPDecomp(
            vector_of_char_t &textBuffer, 
            vector_of_ref_initial_t &factors, uint64_t bytesMemory) :
            textBuffer(textBuffer) {
        
        if(minBytesMemory > bytesMemory) {
            std::cerr << 
                "At least " << minBytesMemory / 1024 / 1024 << 
                "MiB memory are required" << std::endl;
            std::cerr << 
                "[continuing with " << minBytesMemory / 1024 / 1024 << 
                "MiB memory]" << std::endl;
            bytesMemory = minBytesMemory;
        }
        
        cachedPagesPerVector = getNumberOfCachedPages(bytesMemory);
        
        uint64_t bytesVectorsTotal = cachedPagesPerVector * pageSize * blockSize * numberOfVectors;
        sortBytesMemory = bytesMemory - bytesVectorsTotal;
        
        std::cout << "Number of vectors:  " << numberOfVectors << std::endl;
        std::cout << "Memory per vector:  " << bytesVectorsTotal / numberOfVectors / 1024.0 / 1024.0 << "MiB" << std::endl;
        std::cout << "    (block size: " << blockSize / 1024.0 / 1024.0 << "MiB)" << std::endl;
        std::cout << "    (page size:  " << pageSize << " blocks)" << std::endl;
        std::cout << "    (cache size: " << cachedPagesPerVector << " pages)" << std::endl;
        std::cout << "Memory for sorting: " << sortBytesMemory / 1024.0 / 1024.0 << "MiB" << std::endl;
        
        if(textBuffer.numpages() != cachedPagesPerVector) {
            std::cout << std::endl;
            std::cerr << "The given textbuffer has a cache size of " << textBuffer.numpages() << " pages" << std::endl;
            std::cerr << "[continuing execution outside memory specifications]" << std::endl;     
        }
        
        byCopyToV = new vector_of_ref_t(vector_size_t(0), cachedPagesPerVector);
        byCopyToV_new = new vector_of_ref_t(vector_size_t(0), cachedPagesPerVector);
        resolvedV = vector_of_ref_t(vector_size_t(0), cachedPagesPerVector);
        
        byCopyToV_writer = new vector_of_ref_writer_t(*byCopyToV_new);
        resolvedV_writer = new vector_of_ref_writer_t(resolvedV);
        
        std::cout
            << "Converting references from 40 bits to " 
            << bitsPerUInt << " bits..." << std::endl;
            
        vector_of_ref_writer_t toWriter(*byCopyToV);
        for (auto factor : vector_of_ref_reader_initial_t(factors))
            toWriter << factor.template convert<unsigned_t>();
    }
 
    static vector_of_char_t getEmptyTextBuffer(uint64_t totalBytesMemory) {
        uint64_t cachedPages = getNumberOfCachedPages(totalBytesMemory);
        return vector_of_char_t(vector_size_t(0), cachedPages);
    }
 
    vector_of_char_t decompress();
    
};

        
template <unsigned bitsPerUInt>
vector_of_char_t PLCPDecomp<bitsPerUInt>::decompress() {
    std::cout << "Decompressing. Type: " << typeid(unsigned_t).name() << std::endl;
    unsigned scanCount = 0;
    while(byCopyToV->size() > 0) {
        tdc::StatPhase scanPhase("Scan " + std::to_string(++scanCount) + " (Factors: " + std::to_string(byCopyToV->size()) + ")");
        std::cout << std::endl;
        std::cout << "Scan " + std::to_string(scanCount) + " (Factors: " + std::to_string(byCopyToV->size()) + ")" << std::endl;
        scan();
    }
    return textBuffer;
}

template <unsigned bitsPerUInt>
void PLCPDecomp<bitsPerUInt>::scan() {
    //~ count();
    std::cout << "SCAN..." << std::endl;
    
    std::cout << "  Copy from CopyToV to CopyFromV..." << std::endl;
    vector_of_ref_t byCopyFromV = *byCopyToV;

    byCopyToV->push_back(compare_by_copyTo.max_value());
    
    std::cout << "  Sort CopyFromV..." << std::endl;
    stxxl::sort(byCopyFromV.begin(), byCopyFromV.end(), compare_by_copyFrom, sortBytesMemory);
    
    
    std::cout << "  Resolve & Jump..." << std::endl;
    auto toIter = byCopyToV->cbegin();    
    vector_of_ref_reader_t fromReader(byCopyFromV);
    
    //~ uint64_t cmax = 0;
    for (; !fromReader.empty(); ++fromReader)
    {
        auto fromFactor = *fromReader;
        while((*toIter).copyToEnd() <= fromFactor.copyFrom)
            toIter++;
        
        if(!resolve(fromFactor, *toIter)) {
            //~ uint64_t c = 1;
            auto nextToIter = toIter + 1;
            while(!resolve(fromFactor, *nextToIter)) {
                nextToIter++;
                //~ c++;
            }
            //~ if(c > cmax) {
                //~ std::cout << c << " " << std::flush;
                //~ cmax = c;
            //~ }
        };
    }
    std::cout << std::endl;
    byCopyToV_writer->finish();
    resolvedV_writer->finish();
    
    std::cout << "  Update CopyToV..." << std::endl;
    vector_of_ref_t * swap = byCopyToV;
    byCopyToV = byCopyToV_new;
    byCopyToV_new = swap;
    byCopyToV_new->clear();
    delete byCopyToV_writer;
    byCopyToV_writer = new vector_of_ref_writer_t(*byCopyToV_new);
    
    std::cout << "  Sort CopyToV..." << std::endl;
    stxxl::sort(byCopyToV->begin(), byCopyToV->end(), compare_by_copyTo, sortBytesMemory);
    
    restore();
    resolvedV.clear();
    delete resolvedV_writer;
    resolvedV_writer = new vector_of_ref_writer_t(resolvedV);
    
    //~ count();
}

template <unsigned bitsPerUInt>
bool PLCPDecomp<bitsPerUInt>::resolve(ref_t &fromFactor, const ref_t &toFactor) {
    
    //~ std::cout << "(" << fromFactor.copyTo << ", " << fromFactor.copyFrom << ", " << fromFactor.length << "), ";
    //~ std::cout << "(" << toFactor.copyTo << ", " << toFactor.copyFrom << ", " << toFactor.length << ")";
    //~ std::cout << "   ";
    
    bool wasResolve = false;
    unsigned_t prefixLen;    
    if(toFactor.copyTo <= fromFactor.copyFrom) {
        // prefix of fromFactor cannot be resolved
        prefixLen = std::min(fromFactor.length, unsigned_t(toFactor.copyToEnd() - fromFactor.copyFrom));
        auto jumpOffset = fromFactor.copyFrom - toFactor.copyTo;
        ref_t jumped(fromFactor.copyTo, toFactor.copyFrom + jumpOffset, prefixLen);
        (*byCopyToV_writer) << jumped;
        //~ std::cout << "JUMPED:  (" << jumped.copyTo << ", " << jumped.copyFrom << ", " << jumped.length << ")";
        //~ std::cout << "   ";
    } else {
        // prefix of fromFactor can be resolved
        prefixLen = std::min(fromFactor.length, unsigned_t(toFactor.copyTo - fromFactor.copyFrom));
        ref_t resolved(fromFactor.copyTo, fromFactor.copyFrom, prefixLen);
        //restore(resolved);
        (*resolvedV_writer) << resolved;
        wasResolve = true;
        //~ std::cout << "RESOLVED:  (" << resolved.copyTo << ", " << resolved.copyFrom << ", " << resolved.length << ")";
        //~ std::cout << "   ";
    }
    
    if(prefixLen == fromFactor.length) {
        //~ std::cout << "REMAINS: NOTHING" << std::endl;
        return true;
    } else {
        fromFactor.copyTo += prefixLen;
        fromFactor.copyFrom += prefixLen;
        fromFactor.length -= prefixLen;
        //~ std::cout << "REMAINS: (" << fromFactor.copyTo << ", " << fromFactor.copyFrom << ", " << fromFactor.length << ")" << std::endl;
        if(wasResolve) return resolve(fromFactor, toFactor);
        else return false;
    }    
}


template <unsigned bitsPerUInt>
void PLCPDecomp<bitsPerUInt>::restore() {
    std::cout << "  Sort ResolvedV by copyFrom..." << std::endl;
    stxxl::sort(resolvedV.begin(), resolvedV.end(), compare_by_copyFrom, sortBytesMemory);
    
    vector_of_lit_t resolvedLiteralsV(vector_size_t(0), cachedPagesPerVector);
    vector_of_lit_writer_t literalWriter(resolvedLiteralsV);
    
    for (auto factor : resolvedV) {
        auto textIt = textBuffer.cbegin() + factor.copyFrom - 1;
        for (unsigned_t i = 0; i < factor.length; i++)
        {
            literalWriter << lit_t(factor.copyTo + i, *textIt);
            textIt++;
        } 
    }
    literalWriter.finish();
    
    std::cout << "  Sort ResolvedLiteralsV by copyTo..." << std::endl;
    stxxl::sort(resolvedLiteralsV.begin(), resolvedLiteralsV.end(), compare_by_copyTo_lit, sortBytesMemory);
    
    std::cout << "  Restore resolved literals in textbuffer..." << std::endl;
    for (auto literal : resolvedLiteralsV) {
        textBuffer[literal.copyTo - 1] = literal.character;
    }
}

}} //ns