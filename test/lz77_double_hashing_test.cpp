#include <tudocomp/compressors/lz77/LZ77SingleHashing.hpp>
#include <tudocomp/compressors/lz77/LZ77DoubleHashing.hpp>
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

TEST(lz77, SingleHashCompress) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77SingleHashing<coder>>(input, "HASH_BITS=4,MAX_MATCH=5");
    ASSERT_EQ(result.str, "cbabc{3, 5}{3, 5}bc{16, 5}bccbabc{3, 5}{3, 5}bcbabc{3, 3}");
}

TEST(lz77, SingleHashingRoundtrip) {
    const std::string orig = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    const std::string input = "3:5:0c0b0a0b0c13:5:13:5:0b0c116:5:0b0c0c0b0a0b0c13:5:13:5:0b0c0b0a0b0c13:3";
    decompress<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input, orig, "HASH_BITS=4,MAX_MATCH=5");
}

TEST(lz77, DoubleHashCompress) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77DoubleHashing<coder>>(input, "HASH_BITS=4,MAX_MATCH=5");
    ASSERT_EQ(result.str, "cbabc{3, 5}{3, 5}bc{16, 5}bccbabc{3, 5}{3, 5}bcbabc{3, 3}");
}
