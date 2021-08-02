#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include "test/util.hpp"

using namespace tdc;

using coder = lzss::DidacticalCoder;

TEST(lz77, LZ77CompactTries_ExactWindowMatch) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77CompactTries<coder>>(input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "cba{2, 1}{4, 1}{3, 3}{3, 8}{3, 1}{2, 1}{7, 6}{8, 8}{3, 8}{3, 1}{2, 1}{7, 6}");
}

TEST(lz77, LZ77CompactTries_MissingOneCharAtEnd) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<coder>>(input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "cba{2, 1}{4, 1}{3, 3}{3, 8}{3, 1}{2, 1}{7, 6}{8, 8}{3, 8}{3, 1}{5, 1}{7, 5}");
}

TEST(lz77, LZ77CompactTries_StreamingCoder) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    auto result = test::compress<lz77::LZ77CompactTries<coder>>(
            input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "cba{2, 1}{4, 1}{3, 3}{3, 8}{3, 1}{2, 1}{7, 6}{8, 8}{3, 8}{3, 1}{5, 1}{7, 5}");
}

