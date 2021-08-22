#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/compressors/lz77/LZ77StreamingCoder.hpp>
#include <tudocomp/compressors/lz77/LZ77DoubleHashing.hpp>
#include "test/util.hpp"

using namespace tdc;
using coder = lzss::DidacticalCoder;

// python3 get_datasets.py /mnt/c/tudocomp/datasets datasets_config.py --delete_original
// python3 get_datasets.py /mnt/c/tudocomp/datasets large_datasets_config.py --delete_original
// in submodules/datasets
template<class C>
void compressFile(const char *fileIn, const char *fileOut, uint hashBits) {
    std::ostringstream options;
    options << "HASH_BITS=" << hashBits;
    auto m_options = options.str();
    auto m_registry = RegistryOf<Compressor>();

    Input text_in((Path(fileIn)));
    Output encoded_out((Path(fileOut)));

    auto compressor = m_registry.select
            <C>(m_options);
    compressor->compress(text_in, encoded_out);
}

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

template<class C>
void decompressFile(const char *fileIn, const char *fileOut, std::string options) {
    auto m_registry = RegistryOf<Compressor>();

    Input text_in((Path(fileIn)));
    Output decoded_out((Path(fileOut)));


    auto decompressor = m_registry.select<C>(options)->decompressor();
    decompressor->decompress(text_in, decoded_out);
}


TEST(LZ77CompactTries, encode) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.64k.compressed",
                 15);
}

TEST(LZ77CompactTries, decode) {
    decompressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>("/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.64k.compressed",
                                   "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.64k.decompressed",
                                   "HASH_BITS=15");
}