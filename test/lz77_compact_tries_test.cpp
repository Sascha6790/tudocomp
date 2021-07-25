#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>
#include "test/util.hpp"
#include <tudocomp/compressors/lz77/ds/SuffixTree.hpp>
#include <tudocomp/compressors/lz77/ds/WeightedSuffixTree.hpp>
#include <tudocomp/compressors/lz77/ds/CappedWeightedSuffixTree.hpp>

using namespace tdc;

using coder = lzss::DidacticalCoder;

TEST(lz77, LZ77Skeleton) {
    const std::string input = "abcccccccde";
    auto result = test::compress<lz77::LZ77Skeleton<lzss::DidacticalCoder>>(input);
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "abc{2, 6}de");
}

TEST(suffixTree, suffixTree) {
    const std::string buffer = "banana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6, 5, 3, 1, 0, 4, 2};
    int lcp[dsSize] = {0, 0, 1, 3, 0, 0, 2};
    int *pSa = &sa[0];
    int *pLcp = &lcp[0];
    lz77::SuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize);

    WeightedNode<uint> *node0 = st.getRoot()->childNodes['b'];

    WeightedNode<uint> *node4a = st.getRoot()->childNodes['n'];
    WeightedNode<uint> *node4 = node4a->childNodes['$'];
    WeightedNode<uint> *node2 = node4->childNodes['n'];

    WeightedNode<uint> *node5a = st.getRoot()->childNodes['a'];
    WeightedNode<uint> *node5 = node5a->childNodes['$'];
    WeightedNode<uint> *node3a = node5->childNodes['n'];
    WeightedNode<uint> *node3 = node3a->childNodes['$'];
    WeightedNode<uint> *node1 = node3a->childNodes['n'];

    ASSERT_EQ(node0->nodeLabel, 0);
    ASSERT_EQ(node4->nodeLabel, 4);
    ASSERT_EQ(node2->nodeLabel, 2);
    ASSERT_EQ(node5->nodeLabel, 5);
    ASSERT_EQ(node3->nodeLabel, 3);
    ASSERT_EQ(node1->nodeLabel, 1);
}


TEST(wSuffixTree, FIXED_SIZE_TOO_BIG) {
    // FIXED_SIZE * 2 > DS_SIZE
    const std::string buffer = "banana$";
    // zero-based
    const int dsSize = 7;
    int sa[dsSize] = {6, 5, 3, 1, 0, 4, 2};
    int lcp[dsSize] = {0, 0, 1, 3, 0, 0, 2};
    int *pSa = &sa[0];
    int *pLcp = &lcp[0];
    try {
        lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 7);
        FAIL() << "Expected std::invalid_argument";
    } catch (std::invalid_argument const &err) {
        EXPECT_EQ(err.what(), std::string("fixedLength * 2 - 1 > size"));
    }
    catch (...) {
        FAIL() << "Expected std::invalid_argument";
    }
}

TEST(time, cbabcabcabcabca_duration) {
    // zero-based
    const int dsSize = 15;
    int sa[dsSize] = {14, 11, 8, 5, 2, 1, 12, 9, 6, 3, 13, 10, 7, 4, 0};
    int lcp[dsSize] = {0, 1, 4, 7, 10, 0, 1, 3, 6, 9, 0, 2, 5, 8, 1};
    int *pSa = &sa[0];
    int *pLcp = &lcp[0];

    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < 100000; i++) {
        const std::string buffer = "cbabcabcabcabca";
        lz77::WeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize);
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = end - start;
    std::cout << elapsed.count() / 100000 << '\n';
}

TEST(WeightedSuffixTree, cbabcabcabcabca) {
    const std::string buffer = "cbabcabcabcabca";
    // zero-based
    const int dsSize = 15;
    int sa[dsSize] = {14, 11, 8, 5, 2, 1, 12, 9, 6, 3, 13, 10, 7, 4, 0};
    int lcp[dsSize] = {0, 1, 4, 7, 10, 0, 1, 3, 6, 9, 0, 2, 5, 8, 1};
    int *pSa = &sa[0];
    int *pLcp = &lcp[0];
    lz77::WeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize);

    ASSERT_EQ(st.getRoot()->childNodes.size(), 3);

    WeightedNode<uint> *node14 = st.getRoot()->childNodes['a'];
    WeightedNode<uint> *node11 = node14->childNodes['b'];
    WeightedNode<uint> *node8 = node11->childNodes['b'];
    WeightedNode<uint> *node5 = node8->childNodes['b'];
    WeightedNode<uint> *node2 = node5->childNodes['b'];

    WeightedNode<uint> *node1a = st.getRoot()->childNodes['b'];
    WeightedNode<uint> *node1b = node1a->childNodes['a'];
    WeightedNode<uint> *node12 = node1a->childNodes['c'];
    WeightedNode<uint> *node9 = node12->childNodes['b'];
    WeightedNode<uint> *node6 = node9->childNodes['b'];
    WeightedNode<uint> *node3 = node6->childNodes['b'];

    WeightedNode<uint> *node13a = st.getRoot()->childNodes['c'];
    WeightedNode<uint> *node13b = node13a->childNodes['a'];
    WeightedNode<uint> *node0 = node13a->childNodes['b'];
    WeightedNode<uint> *node10 = node13b->childNodes['b'];
    WeightedNode<uint> *node7 = node10->childNodes['b'];
    WeightedNode<uint> *node4 = node7->childNodes['b'];

    // Test label
    ASSERT_EQ(node0->nodeLabel, 0);
    ASSERT_EQ(node1b->nodeLabel, 1);
    ASSERT_EQ(node2->nodeLabel, 2);
    ASSERT_EQ(node3->nodeLabel, 3);
    ASSERT_EQ(node4->nodeLabel, 4);
    ASSERT_EQ(node5->nodeLabel, 5);
    ASSERT_EQ(node6->nodeLabel, 6);
    ASSERT_EQ(node7->nodeLabel, 7);
    ASSERT_EQ(node8->nodeLabel, 8);
    ASSERT_EQ(node9->nodeLabel, 9);
    ASSERT_EQ(node10->nodeLabel, 10);
    ASSERT_EQ(node11->nodeLabel, 11);
    ASSERT_EQ(node12->nodeLabel, 12);
    ASSERT_EQ(node13b->nodeLabel, 13);
    ASSERT_EQ(node14->nodeLabel, 14);
    // test min
    ASSERT_EQ(node0->minLabel, 0);
    ASSERT_EQ(node1a->minLabel, 1);
    ASSERT_EQ(node1b->minLabel, 1);
    ASSERT_EQ(node2->minLabel, 2);
    ASSERT_EQ(node3->minLabel, 3);
    ASSERT_EQ(node4->minLabel, 4);
    ASSERT_EQ(node5->minLabel, 2);
    ASSERT_EQ(node6->minLabel, 3);
    ASSERT_EQ(node7->minLabel, 4);
    ASSERT_EQ(node8->minLabel, 2);
    ASSERT_EQ(node9->minLabel, 3);
    ASSERT_EQ(node10->minLabel, 4);
    ASSERT_EQ(node11->minLabel, 2);
    ASSERT_EQ(node12->minLabel, 3);
    ASSERT_EQ(node13a->minLabel, 0);
    ASSERT_EQ(node13b->minLabel, 4);
    ASSERT_EQ(node14->minLabel, 2);
    // test max
    ASSERT_EQ(node0->maxLabel, 0);
    ASSERT_EQ(node1a->maxLabel, 12);
    ASSERT_EQ(node1b->maxLabel, 1);
    ASSERT_EQ(node2->maxLabel, 2);
    ASSERT_EQ(node3->maxLabel, 3);
    ASSERT_EQ(node4->maxLabel, 4);
    ASSERT_EQ(node6->maxLabel, 6);
    ASSERT_EQ(node5->maxLabel, 5);
    ASSERT_EQ(node7->maxLabel, 7);
    ASSERT_EQ(node8->maxLabel, 8);
    ASSERT_EQ(node9->maxLabel, 9);
    ASSERT_EQ(node10->maxLabel, 10);
    ASSERT_EQ(node11->maxLabel, 11);
    ASSERT_EQ(node12->maxLabel, 12);
    ASSERT_EQ(node13a->maxLabel, 13);
    ASSERT_EQ(node13b->maxLabel, 13);
    ASSERT_EQ(node14->maxLabel, 14);
}

TEST(WSlidingTree, DIFFERENT_MIN_MAX_VALUES_IN_LEAF) {
    // LCP[4] = 5 > FIXED_SIZE = 4
    const std::string buffer = "cbabcabcabcabca";
    // zero-based
    const int dsSize = 15;
    int sa[dsSize] = {};
    int isa[dsSize] = {};
    int lcp[dsSize] = {};
    lz77::LZ77CompactTries<uint>::constructSuffixArray(buffer.c_str(), sa, dsSize);
    lz77::LZ77CompactTries<uint>::constructInverseSuffixArray(sa, isa, dsSize);
    lz77::LZ77CompactTries<uint>::constructLcpArray(buffer.c_str(), sa, isa, lcp, dsSize);
    int *pSa = &sa[0];
    int *pLcp = &lcp[0];
    lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 8);
    st.print(st.getRoot());

    WeightedNode<uint> *node5 = st.getRoot()->childNodes['a'];

    WeightedNode<uint> *node1a = st.getRoot()->childNodes['b'];
    WeightedNode<uint> *node1b = node1a->childNodes['a'];
    WeightedNode<uint> *node6 = node1a->childNodes['c'];

    WeightedNode<uint> *node7a = st.getRoot()->childNodes['c'];
    WeightedNode<uint> *node7b = node7a->childNodes['a'];
    WeightedNode<uint> *node0 = node7a->childNodes['b'];

    // Test label
    ASSERT_EQ(node5->nodeLabel, 5);
    ASSERT_EQ(node1b->nodeLabel, 1);
    ASSERT_EQ(node6->nodeLabel, 6);
    ASSERT_EQ(node7b->nodeLabel, 7);
    ASSERT_EQ(node0->nodeLabel, 0);

    // Test min
    ASSERT_EQ(node5->minLabel, 2);
    ASSERT_EQ(node1a->minLabel, 1);
    ASSERT_EQ(node1b->minLabel, 1);
    ASSERT_EQ(node6->minLabel, 3);
    ASSERT_EQ(node7a->minLabel, 0);
    ASSERT_EQ(node7b->minLabel, 4);
    ASSERT_EQ(node0->minLabel, 0);

    // Test max
    ASSERT_EQ(node5->maxLabel, 5);
    ASSERT_EQ(node1a->maxLabel, 6);
    ASSERT_EQ(node1b->maxLabel, 1);
    ASSERT_EQ(node6->maxLabel, 6);
    ASSERT_EQ(node7a->maxLabel, 7);
    ASSERT_EQ(node7b->maxLabel, 7);
    ASSERT_EQ(node0->maxLabel, 0);
}

TEST(WSlidingTree, abcabcabcbabcab) {
    // LCP[4] = 5 > FIXED_SIZE = 4
    const std::string buffer = "abcabcabcbabcab";
    // zero-based
    const int dsSize = 15;
    int sa[dsSize] = {};
    int isa[dsSize] = {};
    int lcp[dsSize] = {};
    lz77::LZ77CompactTries<uint>::constructSuffixArray(buffer.c_str(), sa, dsSize);
    lz77::LZ77CompactTries<uint>::constructInverseSuffixArray(sa, isa, dsSize);
    lz77::LZ77CompactTries<uint>::constructLcpArray(buffer.c_str(), sa, isa, lcp, dsSize);
    int *pSa = &sa[0];
    int *pLcp = &lcp[0];
    lz77::CappedWeightedSuffixTree<uint> st(pLcp, pSa, buffer.c_str(), dsSize, 8);
    // st.print(st.getRoot());
    //WeightedNode<uint> *node5 = st.getRoot()->childNodes['a'];
}


TEST(lz77, LZ77CompactTries) {
    const std::string input = "cbabcabcabcabcabcbabcab$$";
    auto result = test::compress<lz77::LZ77CompactTries<lzss::DidacticalCoder>>(input, "window=8");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "");
}
