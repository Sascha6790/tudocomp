#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/Tags.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/ds/RingBuffer.hpp>

#include <tudocomp/compressors/lzss/Factor.hpp>
#include <tudocomp/decompressors/LZSSDecompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/compressors/lzss/DidacticalCoder.hpp>
#include <tudocomp/compressors/lzss/FactorizationStats.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>
#include "LZ77Helper.hpp"

namespace tdc {
    namespace lz77 {
        template<typename lzss_coder_t>
        class LZ77Skeleton : public Compressor {

        private:
            size_t minFactorLength{};
            size_t slidingWindowSize{};
            lzss::FactorBufferRAM factors;

        public:
            inline static Meta meta() {
                Meta m(Compressor::type_desc(), "LZ77Skeleton", "Compute LZ77 Factors using Compact Tries");
                m.param("coder", "The output encoder.").strategy<lzss_coder_t>(TypeDesc("lzss_coder"));
                m.param("window", "The sliding window size").primitive(16);
                m.param("threshold", "The minimum factor length.").primitive(2);
                m.inherit_tag<lzss_coder_t>(tags::lossy);
                return m;
            }

            inline LZ77Skeleton() = delete;

            inline explicit LZ77Skeleton(Config &&c) : Compressor(std::move(c)) {
                minFactorLength = this->config().param("threshold").as_uint();
                slidingWindowSize = this->config().param("window").as_uint();
            }

            inline virtual void compress(Input &input, Output &output) override {
                StatPhase root("Root");
                // initialize encoder
                tdc::lzss::DidacticalCoder::Encoder coder = lzss_coder_t(this->config().sub_config("coder")).encoder(
                        output,
                        NoLiterals());

                coder.factor_length_range(Range(minFactorLength, 2 * slidingWindowSize));
                coder.encode_header();

                tdc::io::InputStream stream = input.as_stream();
                int nextChar;

                RingBuffer<uliteral_t> ahead(slidingWindowSize);
                LZ77Helper::initializeAheadBuffer(stream, nextChar, ahead);

                // factorize
                size_t positionInText = 0; // all symbols before positionInText have already been factorized
                RingBuffer<uliteral_t> window(slidingWindowSize);
                StatPhase::wrap("Factorize", [&] {
                    while (!ahead.empty()) {
                        // look for longest prefix of ahead in window
                        size_t factorLength = 1, FactorSource = SIZE_MAX;
                        size_t positionInWindow = 0;

                        for (auto it = window.begin(); it != window.end(); it++) {
                            if (*it == ahead.peek_front()) {
                                // at least one character matches, find factor length
                                // by comparing additional characters
                                size_t len = 1;

                                auto it_window = it + 1;
                                auto it_ahead = ahead.begin() + 1;

                                while (it_window != window.end() &&
                                       it_ahead != ahead.end() &&
                                       *it_window == *it_ahead) {

                                    ++it_window;
                                    ++it_ahead;
                                    ++len;
                                }

                                if (it_window == window.end() && it_ahead != ahead.end()) {
                                    // we looked until the end of the window, but there
                                    // are more characters in the lookahead buffer
                                    // -> test for overlapping factor!
                                    auto it_overlap = ahead.begin();
                                    while (it_ahead != ahead.end() &&
                                           *it_overlap == *it_ahead) {

                                        ++it_overlap;
                                        ++it_ahead;
                                        ++len;
                                    }
                                }

                                // test if factor is currently the longest
                                if (len > factorLength) {
                                    factorLength = len;
                                    FactorSource = positionInWindow;
                                }
                            }
                            ++positionInWindow;
                        }

                        if (factorLength >= minFactorLength) {
                            LZ77Helper::encodeFactor(coder, positionInText, window, factorLength, FactorSource);
                            factors.emplace_back(positionInText, FactorSource, factorLength);
                        } else {
                            LZ77Helper::encodeLiteral(coder, ahead, factorLength);
                        }
                        // advance
                        positionInText += factorLength;
                        LZ77Helper::moveSlidingWindowRight(stream, nextChar, ahead, window, factorLength);
                    }
                    LZ77Helper::printStats(input, root, positionInText, factors);
                });
            }

            inline std::unique_ptr<Decompressor> decompressor() const override {
                return Algorithm::instance<LZSSDecompressor<lzss_coder_t>>();
            }

        };
    }
} //ns
