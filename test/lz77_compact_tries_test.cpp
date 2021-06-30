#include <tudocomp/compressors/lz77/LZ77Skeleton.hpp>
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
