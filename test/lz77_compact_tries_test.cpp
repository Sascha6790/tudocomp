#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>
#include "test/util.hpp"
#include <tudocomp/compressors/lz77/ds/SuffixTree.hpp>
#include <tudocomp/compressors/lz77/ds/CappedWeightedSuffixTree.hpp>

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
TEST(wSuffixTree, uncapped) {
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
TEST(wSuffixTree, FIXED_SIZE_TOO_BIG) {
    // FIXED_SIZE * 2 > DS_SIZE
    const std::string buffer = "banana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6,5,3,1,0,4,2};
    int lcp[dsSize] = {0,0,1,3,0,0,2};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    try {
        lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 7);
        FAIL() << "Expected std::invalid_argument";
    }  catch(std::invalid_argument const & err) {
        EXPECT_EQ(err.what(),std::string("fixedLength * 2 > size"));
    }
    catch(...) {
        FAIL() << "Expected std::invalid_argument";
    }
}

TEST(wSuffixTree, capped_banana$) {
    const std::string buffer = "banana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6,5,3,1,0,4,2};
    int lcp[dsSize] = {0,0,1,3,0,0,2};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 3);
    std::cout << "O";
}

TEST(wSuffixTree, capped_aanana$) {
    const std::string buffer = "aanana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6,5,0,3,1,4,2};
    int lcp[dsSize] = {0,0,0,1,3,0,2};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 3);
    std::cout << "O";
}

TEST(wSuffixTree, LCP_VALUE_EXCEEDS_FIXED_SIZE) {
    // LCP[4] = 5 > FIXED_SIZE = 4
    const std::string buffer = "bananana$";
    // zero-based
    const int dsSize = 9;
    int sa[dsSize] = {8,7,5,3,1,0,6,4,2};
    int lcp[dsSize] = {0,0,1,3,5,0,0,2,4};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 4);
    std::cout << "O";
}

TEST(suffixTree, bananana$) {
    const std::string buffer = "bananana$";
    // zero-based
    const int dsSize = 9;
    int sa[dsSize] = {8,7,5,3,1,0,6,4,2};
    int lcp[dsSize] = {0,0,1,3,5,0,0,2,4};
    int * pSa = &sa[0];
    int * pLcp = &lcp[0];
    lz77::SuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize);
    std::cout << "O";
}




TEST(lz77, LZ77CompactTries) {
    const std::string input = "banana$$$$";
    auto result = test::compress<lz77::LZ77CompactTries<lzss::DidacticalCoder>>(input, "window=3");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "");
}
