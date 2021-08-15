#include "test/util.hpp"
#include <tudocomp/compressors/lz77/LZ77StreamingCoder.hpp>
#include <tudocomp/compressors/lz77/LZ77SuffixSorting.hpp>
#include <tudocomp/compressors/lzss/StreamingCoder.hpp>
#include <tudocomp/compressors/lz77/LZ77DoubleHashing.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/compressors/lz77/LZ77SingleHashing.hpp>
#include <tudocomp/compressors/lz77/LZ77MatchingStatistics.hpp>

#define INTPUT_PATH "/mnt/c/tudocomp/datasets/"
#define OUTPUT_PATH "/mnt/c/tudocomp/output/"
using namespace tdc;

template<class C>
void compressFile(const char *fileIn, const char *fileOut, std::string options) {
    auto m_options = options;
    auto m_registry = RegistryOf<Compressor>();

    Input text_in((Path(fileIn)));
    Output encoded_out((Path(fileOut)));

    auto compressor = m_registry.select
            <C>(m_options);
    compressor->compress(text_in, encoded_out);
}


std::string getOptions(uint bits, uint mode) {
    std::ostringstream options;
    if(bits != 0) {
        options << "HASH_BITS=" << bits;
    }
    if(mode != 0) {
        options << ", COMPRESSION_MODE=" << mode;
    }
    return std::string(options.str());
}

std::string getInput(std::string filename) {
    std::ostringstream fileIn;
    fileIn << INTPUT_PATH << filename;
    return std::string(fileIn.str());
}

std::string getOutput(std::string filename, uint bits, uint mode, std::string extension) {
    std::ostringstream fileOut;
    fileOut << OUTPUT_PATH << filename << "." << bits << "bit" << "." << "mode " << mode <<
            "." << extension;
    return std::string(fileOut.str());
}


std::string files[35] = {
            "cc_commoncrawl.ascii.1MB",
            "cc_commoncrawl.ascii.10MB",
            "cc_commoncrawl.ascii.50MB",
            "cc_commoncrawl.ascii.100MB",
            "cc_commoncrawl.ascii.200MB",
            "pc_sources.1MB",
            "pc_sources.10MB",
            "pc_sources.50MB",
            "pc_sources.100MB",
            "pc_sources.200MB",
            "pc_english.1MB",
            "pc_english.10MB",
            "pc_english.50MB",
            "pc_english.100MB",
            "pc_english.200MB",
            "pc_dblp.xml.1MB",
            "pc_dblp.xml.10MB",
            "pc_dblp.xml.50MB",
            "pc_dblp.xml.100MB",
            "pc_dblp.xml.200MB",
            "pc_dna.1MB",
            "pc_dna.10MB",
            "pc_dna.50MB",
            "pc_dna.100MB",
            "pc_dna.200MB",
            "pcr_kernel.1MB",
            "pcr_kernel.10MB",
            "pcr_kernel.50MB",
            "pcr_kernel.100MB",
            "pcr_kernel.200MB",
            "pcr_para.1MB",
            "pcr_para.10MB",
            "pcr_para.50MB",
            "pcr_para.100MB",
            "pcr_para.200MB"
    };

TEST(LZ77SingleHashing, allFiles) {
    uint bits = 15;
    uint mode = 2;
    for(int i = 0; i < files->size(); i++) {
        std::string filename = files[i];
        compressFile<lz77::LZ77SingleHashing<
                lz77::LZ77StreamingCoder<
                        EliasDeltaCoder,
                        EliasDeltaCoder,
                        EliasDeltaCoder>>>(getInput(filename).c_str(),
                                           getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                           getOptions(bits, mode));
    }
}

TEST(LZ77MatchingStatistics, allaFiles) {
    uint bits = 15;
    uint mode = 2;
    std::string filename = "wiki_all_vital.txt.1MB";
    std::string options = getOptions(bits, mode);
    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       options);
}

TEST(LZ77MatchingStatistics, allaFiles22) {
    uint bits = 15;
    uint mode = 2;
    std::string filename = "wiki_all_vital.txt.1MB";
    compressFile<lz77::LZ77SingleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOptions(bits, mode));
}

TEST(LZ77MatchingStatistics, allaFiles223) {
    uint bits = 15;
    uint mode = 0;
    std::string filename = "wiki_all_vital.txt.1MB";
    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOptions(bits, mode));
}

TEST(LZ77MatchingStatistics, ascii) {
    uint bits = 15;
    uint mode = 0;
    std::string filename = "wiki_all_vital.txt.1MB";
    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    ASCIICoder,
                    ASCIICoder,
                    ASCIICoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOptions(bits, mode));
}

TEST(LZ77DoubleHashing, allFiles) {
    uint bits = 15;
    uint mode = 2;
    for(int i = 0; i < files->size(); i++) {
        std::string filename =  files[i];
        std::string outFile =  "DoubleHashing." + files[i];
        compressFile<lz77::LZ77DoubleHashing<
                lz77::LZ77StreamingCoder<
                        EliasDeltaCoder,
                        EliasDeltaCoder,
                        EliasDeltaCoder>>>(getInput(filename).c_str(),
                                           getOutput(outFile, bits, mode, "elias.compressed").c_str(),
                                           getOptions(bits, mode));
    }
}

