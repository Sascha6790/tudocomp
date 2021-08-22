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
            static void
            printStats(const Input &input, StatPhase &root, size_t positionInText, lzss::FactorBufferRAM factors,
                       uint windowSize) {
                const int defaultWidth = 20;
                const bool showHeaders = false;

                #ifdef FACTORS_ENABLED
                lzss::FactorizationStats stats(factors, positionInText);
                // stats.log();
                if(showHeaders) {
                    col("WindowSize", defaultWidth, showHeaders);
                    col("Factors", defaultWidth, showHeaders);
                    col("Replaced", defaultWidth, showHeaders);
                    col("Unreplaced", defaultWidth, showHeaders);
                    col("Length avg", defaultWidth, showHeaders);
                    col("Length max", defaultWidth, showHeaders);
                    col("memOff", defaultWidth, showHeaders);
                    col("memPeak", defaultWidth, showHeaders);
                    col("memFinal", defaultWidth, showHeaders);
                    col("timeRun", defaultWidth, showHeaders);
                    row();
                }

                std::string lenAvg = std::to_string(stats.len_avg);
                std::replace(lenAvg.begin(), lenAvg.end(), '.', ',');

                col(windowSize, defaultWidth, showHeaders);
                col(stats.num_factors, defaultWidth, showHeaders);
                col(stats.num_replaced, defaultWidth, showHeaders);
                col(stats.num_unreplaced, defaultWidth, showHeaders);
                col(lenAvg, defaultWidth, showHeaders);
                col(stats.len_max, defaultWidth, showHeaders);

                #ifndef STATS_ENABLED
                row();
                #endif
                #endif

                #ifdef STATS_ENABLED
                json obj = root.to_json();
                // std::cout << obj;
                // row();
                #ifdef FACTORS_ENABLED
                    col(std::to_string(static_cast<long>(obj.at("memOff"))), defaultWidth, showHeaders);
                #else
                    col(std::to_string(static_cast<long>(obj.at("memOff"))), defaultWidth, showHeaders, true);
                #endif
                col(std::to_string(static_cast<long>(obj.at("memPeak"))), defaultWidth, showHeaders);
                col(std::to_string(static_cast<long>(obj.at("memFinal"))), defaultWidth, showHeaders);
                std::string timeRun = std::to_string(static_cast<double>(obj.at("timeRun")));
                std::replace(timeRun.begin(), timeRun.end(), '.', ',');
                col(timeRun, defaultWidth, showHeaders);
                row();
                #endif
            }

            static void moveSlidingWindowRight(io::InputStream &stream,
                                               int nextChar,
                                               RingBuffer <uliteral_t> &ahead,
                                               RingBuffer <uliteral_t> &window,
                                               size_t advance) {
                while (advance-- && !ahead.empty()) {
                    window.push_back(ahead.pop_front());
                    if (!stream.eof() && (nextChar = stream.get()) >= 0) {
                        ahead.push_back(uliteral_t(nextChar));
                    }
                }
            }

            static void encodeLiteral(lzss::DidacticalCoder::Encoder &coder,
                                      const RingBuffer <uliteral_t> &ahead,
                                      size_t factorLength) {
                auto positionInAhead = ahead.begin();
                for (size_t charInFactor = 0; charInFactor < factorLength; charInFactor++) {
                    coder.encode_literal(*positionInAhead++);
                }
            }

            static void
            encodeFactor(lzss::DidacticalCoder::Encoder &coder,
                         size_t positionInText,
                         const RingBuffer <uliteral_t> &window,
                         size_t factorLength,
                         size_t FactorSource) {
                coder.encode_factor(
                        lzss::Factor(positionInText, positionInText - window.size() + FactorSource, factorLength));
            }

            static void initializeAheadBuffer(io::InputStream &stream,
                                              int &nextChar,
                                              RingBuffer <uliteral_t> &ahead) {// initialize lookahead buffer
                while (!ahead.full() && (nextChar = stream.get()) >= 0) {
                    ahead.push_back(uliteral_t(nextChar));
                }
            }


            template<typename T>
            static void col(T t, const int &width, const bool showHeaders, bool isFirst = false) {
                if(showHeaders) {
                    std::cout << std::left << std::setw(width) << std::setfill(' ') << t;
                } else {
                    if(!isFirst) {
                        std::cout << '\t';
                    }
                    std::cout << t;
                }
            }


            static void row() {
                std::cout << std::endl;
            }
        };
    }
} //ns
