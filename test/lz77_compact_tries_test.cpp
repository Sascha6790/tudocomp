#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/compressors/lzss/StreamingCoder.hpp>
#include "test/util.hpp"

using namespace tdc;


TEST(lz77, LZ77CompactTries_ExactWindowMatch) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}

TEST(lz77, LZ77CompactTries_MissingOneCharAtEnd) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}

TEST(lz77, LZ77CompactTries_StreamingCoder) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}


TEST(lz77, LZ77CompactTries_BinaryCoder) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<BinaryCoder, BinaryCoder, BinaryCoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}



TEST(lz77, LZ77CompactTries_EliasDelta) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<EliasDeltaCoder, EliasDeltaCoder, EliasDeltaCoder>>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    result.assert_decompress();
}

