#include <tudocomp/compressors/LZ77WithoutWindow.hpp>
#include "test/util.hpp"

using namespace tdc;

TEST(lz77, isa1) {
    auto compressor = tdc::create_algo<
            LZ77WithoutWindow<TextDS<SADivSufSort, PhiFromSA, PLCPFromPhi, LCPFromPLCP, ISAFromSA>>>();

    test::TestInput input = test::compress_input("abcccccccde");
    test::TestOutput output = test::compress_output();
    compressor.compress(input, output);
}

TEST(lz77, isa2) {
    auto compressor = tdc::create_algo<
            LZ77WithoutWindow<TextDS<SADivSufSort, PhiFromSA, PLCPFromPhi, LCPFromPLCP, SparseISA<SADivSufSort>>>>();

    test::TestInput input = test::compress_input("abcccccccde");
    test::TestOutput output = test::compress_output();
    compressor.compress(input, output);
}