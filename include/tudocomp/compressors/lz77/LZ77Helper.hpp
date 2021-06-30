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

#ifndef STATS_DISABLED
#define STATS_ENABLED
#endif

namespace tdc {
    namespace lz77 {
       class LZ77Helper {
        public:
            static void printStats(const Input &input, StatPhase &root, size_t positionInText, lzss::FactorBufferRAM factors) {
                json obj = root.to_json();
                std::cout << obj;
                row();
                row();
                row();

                lzss::FactorizationStats stats(factors, positionInText);
                stats.log();

                col("Factors", 10);
                col("Replaced", 10);
                col("Unreplaced", 12);
                col("memPeak", 10);
                col("memFinal", 10);
                col("timeDelta", 12);
                col("Text[0..19]", 20);
                col("Encoded", 22);
                row();
                col(stats.num_factors, 10);
                col(stats.num_replaced, 10);
                col(stats.num_unreplaced, 12);
                col(std::to_string(static_cast<long>(obj.at("memPeak"))), 10);
                col(std::to_string(static_cast<long>(obj.at("memFinal"))), 10);
                col(std::to_string(static_cast<double>(obj.at("timeDelta"))), 12);
                col(input.as_view().slice(0, std::min((int) input.size(), 20)).data(), 20);
            }

            static void moveSlidingWindowRight(io::InputStream &stream,
                                        int nextChar,
                                        RingBuffer<uliteral_t> &ahead,
                                        RingBuffer<uliteral_t> &window,
                                        size_t advance) {
                while (advance-- && !ahead.empty()) {
                    window.push_back(ahead.pop_front());
                    if (!stream.eof() && (nextChar = stream.get()) >= 0) {
                        ahead.push_back(uliteral_t(nextChar));
                    }
                }
            }

            static void encodeLiteral(lzss::DidacticalCoder::Encoder &coder,
                               const RingBuffer<uliteral_t> &ahead,
                               size_t factorLength) {
                auto positionInAhead = ahead.begin();
                for (size_t charInFactor = 0; charInFactor < factorLength; charInFactor++) {
                    coder.encode_literal(*positionInAhead++);
                }
            }

            static void
            encodeFactor(lzss::DidacticalCoder::Encoder &coder,
                         size_t positionInText,
                         const RingBuffer<uliteral_t> &window,
                         size_t factorLength,
                         size_t FactorSource) {
                coder.encode_factor(
                        lzss::Factor(positionInText, positionInText - window.size() + FactorSource, factorLength));
            }

            static void initializeAheadBuffer(io::InputStream &stream,
                                       int &nextChar,
                                       RingBuffer<uliteral_t> &ahead) {// initialize lookahead buffer
                while (!ahead.full() && (nextChar = stream.get()) >= 0) {
                    ahead.push_back(uliteral_t(nextChar));
                }
            }



            template<typename T>
            static void col(T t, const int &width) {
                std::cout << std::left << std::setw(width) << std::setfill(' ') << t;
            }

            static void row() {
                std::cout << std::endl;
            }
        };
    }
} //ns
