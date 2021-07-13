#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>
#include "test/util.hpp"
#include <tudocomp/compressors/lz77/ds/SuffixTree.hpp>

using namespace tdc;

using coder = lzss::DidacticalCoder;

TEST(lz77, LZ77Skeleton)
{
    const std::string input = "abcccccccde";
    auto result = test::compress<lz77::LZ77Skeleton<lzss::DidacticalCoder>>(input);
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "abc{2, 6}de");
}

TEST(suffixTree, suffixTree) {
    const std::string buffer = "banana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6,5,3,1,0,4,2};
    int lcp[dsSize] = {0,0,1,3,0,0,2};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    lz77::SuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize);
}
TEST(wSuffixTree, wSuffixTree) {
    const std::string buffer = "banana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6,5,3,1,0,4,2};
    int lcp[dsSize] = {0,0,1,3,0,0,2};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    lz77::WeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize);
    std::cout << "OK";
}

TEST(lz77, LZ77CompactTries) {
    const std::string input = "banana$$$$";
    auto result = test::compress<lz77::LZ77CompactTries<lzss::DidacticalCoder>>(input, "window=3");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "");
}
