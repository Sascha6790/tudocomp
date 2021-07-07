#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/Tags.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/util/divsufsort.hpp>

#include <tudocomp/ds/RingBuffer.hpp>

#include <tudocomp/compressors/lzss/Factor.hpp>
#include <tudocomp/decompressors/LZSSDecompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/compressors/lzss/DidacticalCoder.hpp>
#include <tudocomp/compressors/lzss/FactorizationStats.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>
#include <divsufsort.h>
#include "tudocomp/compressors/lz77/LZ77Helper.hpp"
#include "tudocomp/compressors/lz77/ds/WExponentialSearchTree.hpp"

namespace tdc {
    namespace lz77 {
        template<typename lzss_coder_t>
        class LZ77CompactTries : public Compressor {

        private:
            size_t minFactorLength{};
            size_t windowSize{};
            lzss::FactorBufferRAM factors;
            int *suffixArray;
            int *inverseSuffixArray;
            int *lcp;
            int i, j, h;

        public:
            inline static Meta meta() {
                Meta m(Compressor::type_desc(), "LZ77CompactTries", "Compute LZ77 Factors using Compact Tries");
                m.param("coder", "The output encoder.").strategy<lzss_coder_t>(TypeDesc("lzss_coder"));
                m.param("window", "The sliding window size").primitive(16);
                m.param("threshold", "The minimum factor length.").primitive(2);
                m.inherit_tag<lzss_coder_t>(tags::lossy);
                return m;
            }

            inline LZ77CompactTries() = delete;

            inline explicit LZ77CompactTries(Config &&c) : Compressor(std::move(c)) {
                minFactorLength = this->config().param("threshold").as_uint();
                windowSize = this->config().param("window").as_uint();
            }

            inline void compress(Input &input, Output &output) override {
                StatPhase root("Root");
                // initialize encoder
                tdc::lzss::DidacticalCoder::Encoder coder = lzss_coder_t(this->config().sub_config("coder")).encoder(
                        output,
                        NoLiterals());

                coder.factor_length_range(Range(minFactorLength, 2 * windowSize));
                coder.encode_header();

                tdc::io::InputStream stream = input.as_stream();


                // Divide Text S into blocks of length w
                // | ---- block 1 ---- | ---- block 2 ---- |
                // Build Patricia Tree where each suffix has a fixed length of w
                // Having a Patricia Tree with a static suffix length requires to build a Patricia Tree with an input of size 2w and truncating each suffix at length w
                // This requires a look ahead of length w. block 1 uses block 2 as look-ahead buffer
                // | ---- block 1 ---- | ---- block 2 ---- | ---- l-ahead ---- |
                // Since we don't want to deal with three buffers, we'll just use one buffer with a size of 3w.
                // RingBuffer<uliteral_t> buffer(3*windowSize);
                int bufSize = 3 * windowSize + 1; // +1 for terminating bit
                int dsSize = 2 * windowSize;
                char *buffer = new char[bufSize];
                suffixArray = new int[dsSize];
                inverseSuffixArray = new int[dsSize];
                lcp = new int[dsSize];

                assert(buffer);

                while (stream.good()) {
                    std::cout << "Stream id good" << std::endl;

                    stream.get(&*buffer, bufSize);
                    if (stream.eof()) {
                        std::cout << "EOF" << std::endl;
                    } else {
                        //SA-Block-1
                        constructSaFromBuffer(buffer, dsSize);
                        constructLcpFromSa(buffer, dsSize);

                        debug(dsSize);
                    }
                    std::cout << "Number of read chars: " << stream.gcount() << std::endl;
                }
                std::cout << "Stream id not good (anymore)" << std::endl;
                std::cout << "Cleanup" << std::endl;
                delete[] buffer;
                delete[] suffixArray;
            }

            void debug(int dsSize) {
                for (i = 0; i < dsSize; i++) {
                    std::cout << suffixArray[i] << std::endl;
                }
                std::cout << std::endl;
                for (i = 0; i < dsSize; i++) {
                    std::cout << inverseSuffixArray[i] << std::endl;
                }
                std::cout << std::endl;
                for (i = 0; i < dsSize; i++) {
                    std::cout << lcp[i] << std::endl;
                }
            }

            void constructSuffixArray(const char *buffer, int dsSize) const { divsufsort(reinterpret_cast<const sauchar_t *> (buffer),
                                                                                         suffixArray, dsSize); }

            void constructLcp(const char *buffer, int dsSize) {

                for (i = 0; i < dsSize; i++) {
                    inverseSuffixArray[suffixArray[i]] = i;
                }

                h = 0;
                lcp[0] = 0;
                // calculate height
                for (i = 0; i < dsSize; i++) {
                    if (inverseSuffixArray[i] > 0) {
                        j = suffixArray[inverseSuffixArray[i] - 1];
                        while (std::max(i + h, j + h) < dsSize && buffer[i + h] == buffer[j + h]) {
                            h++;
                        }
                        lcp[inverseSuffixArray[i]] = h;
                        if (h > 0) {
                            h--;
                        }
                    }
                }
            }

            inline std::unique_ptr<Decompressor> decompressor() const override {
                return Algorithm::instance<LZSSDecompressor<lzss_coder_t>>();
            }

        };
    }
} //ns
