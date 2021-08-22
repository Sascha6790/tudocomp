#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include "test/util.hpp"

using namespace tdc;

auto didacticalCompressor = [](std::string input, std::string options) {
    return test::compress<lz77::LZ77CompactTries<lzss::DidacticalCoder>>(input, options);
};

auto eliasCompressor = [](std::string input, std::string options) {
    return test::compress<lz77::LZ77CompactTries<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input,
                                                                                                                options);
};

TEST(LZ77CompactTries, factorOfLength2GetsLiteral) {
    const std::string input = "cbabcabdcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = didacticalCompressor(input, "HASH_BITS=3");
    ASSERT_EQ(result.str, "cbabcabd{4, 3}{3, 5}bcb{7, 5}c{8, 7}{3, 8}bcbabcabc");
}


TEST(LZ77CompactTries, noRemainerInWindow) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    eliasCompressor(input, "HASH_BITS=3").assert_decompress();
}

TEST(LZ77CompactTries, remainerInWindow) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    eliasCompressor(input, "HASH_BITS=3").assert_decompress();
}

TEST(LZ77CompactTries, windowGreaterThanStream) {
const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
eliasCompressor(input, "HASH_BITS=6").assert_decompress();
}


TEST(LZ77CompactTries, HASH_BITS_3) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    eliasCompressor(input, "HASH_BITS=3").assert_decompress();
}


TEST(LZ77CompactTries, HASH_BITS_4) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    eliasCompressor(input, "HASH_BITS=4").assert_decompress();
}

TEST(LZ77CompactTries, readResizedBlockAtEnd) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcab";
    eliasCompressor(input, "HASH_BITS=5").assert_decompress();
}

TEST(LZ77CompactTries, minMatch4) {
    const std::string input = "cbabcabdcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = didacticalCompressor(input, "HASH_BITS=3,MIN_MATCH=4");
    ASSERT_EQ(result.str, "cbabcabdcab{3, 5}bcb{7, 5}c{8, 7}{3, 8}bcbabcabc");
}

TEST(LZ77CompactTries, minMatch2) {
    const std::string input = "cbabcabdcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = didacticalCompressor(input, "HASH_BITS=3,MIN_MATCH=2");
    ASSERT_EQ(result.str, "cbabc{3, 2}d{4, 3}{3, 5}{3, 2}b{7, 5}c{8, 7}{3, 8}{3, 2}ba{4, 2}{3, 2}c");
}

TEST(LZ77CompactTries, literalIsFactor) {
    const std::string input = "cbabcabdcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = didacticalCompressor(input, "HASH_BITS=3,MIN_MATCH=1");
    ASSERT_EQ(result.str, "cba{2, 1}{4, 1}{3, 2}d{4, 3}{3, 5}{3, 2}{2, 1}{7, 5}{3, 1}{8, 7}{3, 8}{3, 2}{2, 1}{4, 1}{4, 2}{3, 2}c");
}


