#include <tudocomp/compressors/lzss/DidacticalCoder.hpp>
#include <tudocomp/compressors/lz77/LZ77SuffixSorting.hpp>
#include "test/util.hpp"

using namespace tdc;
using coder = lzss::DidacticalCoder;

template<class C>
void decompress(const std::string compressedTest, std::string originalText, std::string options) {
    std::vector<uint8_t> decoded_buffer;
    {
        Input text_in = Input::from_memory(compressedTest);
        Output decoded_out = Output::from_memory(decoded_buffer);

        auto m_registry = RegistryOf<Compressor>();
        auto decompressor = m_registry.select<C>(options)->decompressor();
        decompressor->decompress(text_in, decoded_out);
    }
    std::string decompressed_text{
            decoded_buffer.begin(),
            decoded_buffer.end(),
    };
    ASSERT_EQ(originalText, decompressed_text);
}

TEST(lz77, SuffixSortingInitialTest) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77SuffixSorting<lzss::DidacticalCoder>>(input, "HASH_BITS=4");
    ASSERT_EQ(result.str, "cbabc{3, 12}b{7, 6}cb{8, 6}{6, 8}cb{7, 6}");
}