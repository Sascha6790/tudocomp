#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
#include <tudocomp/compressors/lz77/LZ77CompactTries.hpp>
#include <tudocomp/compressors/lz77/ds/WExponentialSearchTree.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>
#include "test/util.hpp"

using namespace tdc;

using coder = lzss::DidacticalCoder;

TEST(lz77, LZ77Skeleton)
{
    const std::string input = "abcccccccde";
    auto result = test::compress<lz77::LZ77Skeleton<lzss::DidacticalCoder>>(input);
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "abc{2, 6}de");
}

TEST(lz77, LZ77CompactTries) {
    const std::string input = "abcccccccde";
    auto result = test::compress<lz77::LZ77CompactTries<lzss::DidacticalCoder>>(input, "window=3");
    std::cout << std::left << std::setw(22) << std::setfill(' ') << result.str;
    ASSERT_EQ(result.str, "");
}


TEST(lz77, WExponentialSearchTree_banana$) {
    char buffer[] = "banana$$$$$$$$$$$$";
    size_t window = 7;
    auto *pSearchTree = new lz77::WExponentialSearchTree(8);
    for (unsigned int i = 0; i < window; i++) {
        pSearchTree->add(&buffer[i], window);
    }
    Node *tree = pSearchTree->getTree();
    lz77::WExponentialSearchTree::printChildren(tree->childNodes);
    std::cout << std::endl << "Node( [$$$$$$$][a--> Node( [$$$$$$][na--> Node( [$$$$][na$$] ) ] ) ][na--> Node( [$$$$$][na$$$] ) ][banana$] ) ";
}

TEST(lz77, WExponentialSearchTree_mississippi$) {
    char buffer[] = "mississippi$$$$$$$$$$$$$$$$$$";
    size_t window = 12;
    auto *pSearchTree = new lz77::WExponentialSearchTree(8);
    for (unsigned int i = 0; i < window; i++) {
        pSearchTree->add(&buffer[i], window);
    }
    Node *tree = pSearchTree->getTree();
    lz77::WExponentialSearchTree::printChildren(tree->childNodes);
    std::cout << std::endl << "Node( [$$$$$$$$$$$$][p--> Node( [i$$$$$$$$$$][pi$$$$$$$$$] ) ][i--> Node( [$$$$$$$$$$$][ppi$$$$$$$$][ssi--> Node( [ppi$$$$$][ssippi$$] ) ] ) ][s--> Node( [i--> Node( [ppi$$$$$$$][ssippi$$$$] ) ][si--> Node( [ppi$$$$$$][ssippi$$$] ) ] ) ][mississippi$] ) ";
}

TEST(lz77, WExponentialSearchTree_ABAB) {
    char buffer[] = "ABAB$$$$$$";
    size_t window = 5;
    auto *pSearchTree = new lz77::WExponentialSearchTree(8);
    for (unsigned int i = 0; i < window; i++) {
        pSearchTree->add(&buffer[i], window);
    }
    Node *tree = pSearchTree->getTree();
    lz77::WExponentialSearchTree::printChildren(tree->childNodes);
    std::cout << std::endl << "Node( [$$$$$][B--> Node( [$$$$][AB$$] ) ][AB--> Node( [$$$][AB$] ) ] ) ";
}
TEST(lz77, WExponentialSearchTree_GTCCGAAGCTCCGG$) {
    char buffer[] = "GTCCGAAGCTCCGG$$$$$$$$$$$$$$$$$";
    size_t window = 15;
    auto *pSearchTree = new lz77::WExponentialSearchTree(8);
    for (unsigned int i = 0; i < window; i++) {
        pSearchTree->add(&buffer[i], window);
    }
    Node *tree = pSearchTree->getTree();
    lz77::WExponentialSearchTree::printChildren(tree->childNodes);
    std::cout << std::endl << "Node( [$$$$$$$$$$$$$$$][A--> Node( [GCTCCGG$$$$$$$][AGCTCCGG$$$$$$] ) ][C--> Node( [TCCGG$$$$$$$$$][G--> Node( [G$$$$$$$$$$$$][AAGCTCCGG$$$$] ) ][CG--> Node( [G$$$$$$$$$$$][AAGCTCCGG$$$] ) ] ) ][TCCG--> Node( [G$$$$$$$$$$][AAGCTCCGG$$] ) ][G--> Node( [$$$$$$$$$$$$$$][CTCCGG$$$$$$$$][AAGCTCCGG$$$$$][G$$$$$$$$$$$$$][TCCGAAGCTCCGG$] ) ] ) ";
}
