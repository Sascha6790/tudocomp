#include "test/util.hpp"
#include <tudocomp/compressors/lz77/LZ77StreamingCoder.hpp>
#include <tudocomp/compressors/lz77/LZ77SuffixSorting.hpp>
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
    if (bits != 0) {
        options << "HASH_BITS=" << bits;
    }
    if (mode != 0) {
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

const int numberOfRelevantFiles = 12;
std::string relevantFiles[numberOfRelevantFiles] = {
        "fields.c",
        "cp.html",
        "kennedy.xls",
        "bible.txt",
        "E.coli",
        "cc_commoncrawl.ascii.10MB",
        "pc_dblp.xml.10MB",
        "pc_dna.10MB",
        "pc_english.10MB",
        "pc_sources.10MB",
        "pcr_kernel.10MB",
        "pcr_para.10MB"
};

const int numberOfBits = 6;
int bitCounts[numberOfBits] = {15, 16, 17, 18, 19, 20};

// For temporary tests.
// TEST(debug, debugHtml) {
//     uint mode = 0;
//     std::string suffix("LZ77SingleHashingMode1.elias.compressed");
//     uint bits = 15;
//     std::string filename("fields.c");
//
//     // for (auto &bits : bitCounts) {
//     std::cout << "debugHtml \t" << filename;
//     compressFile<lz77::LZ77DoubleHashing<lz77::LZ77StreamingCoder<
//             EliasDeltaCoder,
//             EliasDeltaCoder,
//             EliasDeltaCoder>>>(getInput(filename).c_str(),
//                                getOutput(filename, bits, mode, suffix).c_str(),
//                                getOptions(bits, mode));
//     // }
// }

TEST(_RELEVANT_FILES_, LZ77SingleHashingMode1) {
    uint mode = 1;
    std::string suffix("LZ77SingleHashingMode1.elias.compressed");

    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77SingleHashing Mode 1\t" << filename;
            compressFile<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}

TEST(_RELEVANT_FILES_, LZ77SingleHashingMode2) {
    uint mode = 2;
    std::string suffix("LZ77SingleHashingMode2.elias.compressed");


    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77SingleHashing Mode 2\t" << filename;
            compressFile<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}

TEST(_RELEVANT_FILES_, LZ77CompactTries) {
    uint mode = 0;
    std::string suffix("LZ77CompactTries.elias.compressed");


    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77CompactTries\t" << filename;
            compressFile<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}


TEST(_RELEVANT_FILES_, LZ77DoubleHashing_Mode1) {
    uint mode = 1;
    std::string suffix("LZ77DoubleHashing_Mode1.elias.compressed");


    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77DoubleHashing Mode 1\t" << filename;
            compressFile<lz77::LZ77DoubleHashing<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}

TEST(_RELEVANT_FILES_, LZ77MatchingStatistics) {
    uint mode = 0;
    std::string suffix("LZ77MatchingStatistics.elias.compressed");


    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77MatchingStatistics\t" << filename;
            compressFile<lz77::LZ77MatchingStatistics<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}

TEST(_RELEVANT_FILES_, LZ77SuffixSorting) {
    uint mode = 0;
    std::string suffix("LZ77SuffixSorting.elias.compressed");


    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77SuffixSorting\t" << filename;
            compressFile<lz77::LZ77SuffixSorting<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}

TEST(_RELEVANT_FILES_, LZ77DoubleHashingMode2) {
    uint mode = 2;
    std::string suffix("LZ77DoubleHashingM2.elias.compressed");


    for (auto &file : relevantFiles) {
        for (auto &bits : bitCounts) {
            std::string filename = file;
            std::cout << "LZ77DoubleHashing Mode 2\t" << filename;
            compressFile<lz77::LZ77DoubleHashing<lz77::LZ77StreamingCoder<
                    EliasDeltaCoder,
                    EliasDeltaCoder,
                    EliasDeltaCoder>>>(getInput(filename).c_str(),
                                       getOutput(filename, bits, mode, suffix).c_str(),
                                       getOptions(bits, mode));

            std::ifstream in(getInput(filename), std::ifstream::ate | std::ifstream::binary);
            int fileSize = in.tellg();
            in.close();
            std::cout << "\t" << fileSize;

            std::ifstream out(getOutput(filename, bits, mode, suffix), std::ifstream::ate | std::ifstream::binary);
            fileSize = out.tellg();
            out.close();
            std::cout << "\t" << fileSize << std::endl;
        }
    }
}
