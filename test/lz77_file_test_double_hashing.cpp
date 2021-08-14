#include <tudocomp/compressors/lz77/LZ77StreamingCoder.hpp>
#include <tudocomp/compressors/lz77/LZ77SingleHashing.hpp>
#include <tudocomp/compressors/lz77/LZ77DoubleHashing.hpp>
#include <tudocomp/compressors/lzss/StreamingCoder.hpp>
#include "test/util.hpp"

#define INTPUT_PATH "/mnt/c/tudocomp/datasets/"
#define OUTPUT_PATH "/mnt/c/tudocomp/output/"
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

std::string getOptions(uint bits, uint mode) {
    std::ostringstream options;
    options << "HASH_BITS=" << bits << ", COMPRESSION_MODE=" << mode;
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


TEST(wiki_all_vital_1Mb_binary_compress, MODE_1) {
    uint bits = 15;
    uint mode = 1;
    std::string filename = "wiki_all_vital.txt.1MB";

    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOptions(bits, mode));
}

TEST(wiki_all_vital_1Mb_binary_compress, MODE_2) {
    uint bits = 15;
    uint mode = 2;
    std::string filename = "wiki_all_vital.txt.1MB";

    compressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOptions(bits, mode));
}


TEST(wiki_all_vital_1Mb_binary_decompress, MODE_1) {
    uint bits = 15;
    uint mode = 1;
    std::string filename = "wiki_all_vital.txt.1MB";
    decompressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOutput(filename, bits, mode, "elias.decompressed").c_str(),
                                       getOptions(bits, mode));
}


TEST(wiki_all_vital_1Mb_binary_decompress, MODE_2) {
    uint bits = 15;
    uint mode = 2;
    std::string filename = "wiki_all_vital.txt.1MB";
    decompressFile<lz77::LZ77DoubleHashing<
            lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getOutput(filename, bits, mode, "elias.compressed").c_str(),
                                       getOutput(filename, bits, mode, "elias.decompressed").c_str(),
                                       getOptions(bits, mode));
}
