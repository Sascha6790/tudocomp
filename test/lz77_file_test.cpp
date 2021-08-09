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
void compressFile(const char *fileIn, const char *fileOut, uint windowSize) {
    std::ostringstream options;
    options << "window=" << windowSize;
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


TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_MultipleWindowSizes) {
    for (int i = 4000; i <= 256000; i += 4000) {
        std::ostringstream outFile;
        outFile << "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii." << i << ".compressed";
        compressFile<lz77::LZ77CompactTries<
                lz77::LZ77StreamingCoder<
                        BinaryCoder,
                        BinaryCoder,
                        BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                     outFile.str().c_str(),
                     i);
    }
}

TEST(LZ77CompactTries, pc_dblp_xml_10MB_series) {
    for (int i = 4000; i <= 128000; i += 4000) {
        std::ostringstream outFile;
        outFile << "/mnt/c/tudocomp/output/pc_dblp.xml.10MB.ascii." << i << ".compressed";
        compressFile<lz77::LZ77CompactTries<
                lz77::LZ77StreamingCoder<
                        BinaryCoder,
                        BinaryCoder,
                        BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_dblp.xml.10MB",
                     outFile.str().c_str(),
                     i);
    }
}

TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_16k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.16k.compressed",
                 16000);
}

TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_32k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.32k.compressed",
                 32000);
}

TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_64k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.64k.compressed",
                 64000);
}

TEST(LZ77CompactTries, pc_proteins_10MB_ascii_compress_64k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.10MB",
                 "/mnt/c/tudocomp/output/pc_proteins.10MB.ascii.64k.compressed",
                 64000);
}


TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_80k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.80k.compressed",
                 80000);
}

TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_96k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.96k.compressed",
                 96000);
}

TEST(LZ77CompactTries, pc_proteins_1MB_ascii_compress_128k) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/pc_proteins.1MB",
                 "/mnt/c/tudocomp/output/pc_proteins.1MB.ascii.128k.compressed",
                 128000);
}

TEST(LZ77CompactTries, large_commoncrawl_1Mb_compress) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/large_commoncrawl_10240.ascii.1MB",
                 "/mnt/c/tudocomp/output/large_commoncrawl_10240.ascii.1MB.64k.compressed",
                 64000);
}

TEST(LZ77CompactTries, wiki_all_vital_txt_200MB) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/wiki_all_vital.txt.200MB",
                 "/mnt/c/tudocomp/output/wiki_all_vital.txt.200MB.64k.compressed",
                 64000);
}

TEST(LZ77CompactTries, commonCrawl10GB) {
    compressFile<lz77::LZ77CompactTries<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/large_commoncrawl_10240.ascii.10240MB",
                 "/mnt/c/tudocomp/output/large_commoncrawl_10240.ascii.10240MB.64k.compressed",
                 64000);
}


TEST(LZ77DoubleHashing, wiki_all_vital_1Mb_binary_compress) {
    uint bits = 15;
    uint maxMatch = 258;
    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/datasets/wiki_all_vital.txt.ascii.1MB",
                                   "/mnt/c/tudocomp/output/wiki_all_vital.binary.1MB.15bit.compressed",
                                   "HASH_BITS=15,MAX_MATCH=258");
}
TEST(LZ77DoubleHashing, wiki_all_vitall_1Mb_ascii_compress) {
    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    ASCIICoder,
                    ASCIICoder,
                    ASCIICoder>>>("/mnt/c/tudocomp/datasets/wiki_all_vital.txt.1MB",
                                   "/mnt/c/tudocomp/output/wiki_all_vital.ascii.1MB.15bit.compressed",
                                   "HASH_BITS=15,MAX_MATCH=258");
}

TEST(LZ77DoubleHashing, wiki_all_vital_1Mb_binary_decompress) {
    decompressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    BinaryCoder,
                    BinaryCoder,
                    BinaryCoder>>>("/mnt/c/tudocomp/output/wiki_all_vital.binary.1MB.15bit.compressed",
                                   "/mnt/c/tudocomp/output/wiki_all_vital.binary.1MB.15bit.decompressed",
                                   "HASH_BITS=15,MAX_MATCH=258");
}
TEST(LZ77DoubleHashing, wiki_all_vital_1Mb_ascii_decompress) {
    decompressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    ASCIICoder,
                    ASCIICoder,
                    ASCIICoder>>>("/mnt/c/tudocomp/output/wiki_all_vital.ascii.1MB.15bit.compressed",
                                   "/mnt/c/tudocomp/output/wiki_all_vital.ascii.1MB.15bit.decompressed",
                                   "HASH_BITS=15,MAX_MATCH=258");
}