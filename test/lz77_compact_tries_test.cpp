#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/compressors/lzss/StreamingCoder.hpp>
#include "test/util.hpp"

using namespace tdc;


TEST(LZ77CompactTries, ExactWindowMatch_W8) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}

TEST(LZ77CompactTries, MissingOneCharAtEnd_W8) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}

TEST(LZ77CompactTries, ASCIICoder_W8) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}


TEST(LZ77CompactTries, BinaryCoder_W8) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<BinaryCoder, BinaryCoder, BinaryCoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}



TEST(LZ77CompactTries, LZ77CompactTries_EliasDelta) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<EliasDeltaCoder, EliasDeltaCoder, EliasDeltaCoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}

