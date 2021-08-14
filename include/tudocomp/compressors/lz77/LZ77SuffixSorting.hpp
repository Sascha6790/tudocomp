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
#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/compressors/lz77/LZ77StreamingCoder.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>
#include <tudocomp/coders/EliasDeltaCoder.hpp>
#include "tudocomp/compressors/lz77/LZ77Helper.hpp"
#include "tudocomp/compressors/lz77/ds/CappedWeightedSuffixTree.hpp"

namespace tdc::lz77 {

    template<typename lz77_coder>
    class [[maybe_unused]] LZ77SuffixSorting : public Compressor {

    private:
        const uint HASH_BITS; // dSize: pow(2,HASH_BITS)
        const uint WINDOW_SIZE; // default: 2^15     size of dictionary

        char *window;
        int *suffixArray;
        int *inverseSuffixArray;
        int *lcpArray;

        const float ALPHA = 0.5;
        const uint MIN_MATCH = 3;
        const uint MIN_LOOKAHEAD;
        uint DS_SIZE; // should be const but it might change if the stream is too small. (edge case..)

        #ifdef STATS_ENABLED
        lzss::FactorBufferRAM factors;
        lzss::FactorBufferRAM *fac;
        std::streampos streampos = 0;
        #endif

    public:
        inline static Meta meta() {
            Meta m(Compressor::type_desc(), "LZ77CompactTries", "Compute LZ77 Factors using Compact Tries");
            m.param("coder", "The output encoder.").strategy<lz77_coder>(TypeDesc("lzss_coder"));
            m.param("HASH_BITS", "dictionary of size 2^HASH_BITS").primitive(3);
            m.param("MIN_MATCH", "minimum factor size").primitive(3);
            m.param("MAX_MATCH", "maximum factor size").primitive(258);
            m.inherit_tag<lz77_coder>(tags::lossy);
            return m;
        }


        inline explicit LZ77SuffixSorting(Config &&c) : Compressor(std::move(c)),
                                                        HASH_BITS(this->config().param("HASH_BITS").as_uint()),
                                                        WINDOW_SIZE(1 << HASH_BITS),
                                                        MIN_LOOKAHEAD(WINDOW_SIZE),
                                                        DS_SIZE((int) ((1 + ALPHA) * WINDOW_SIZE)) {
            #ifdef STATS_ENABLED
            fac = &factors;
            #endif

            assert(MIN_LOOKAHEAD <= WINDOW_SIZE);
        }

        inline LZ77SuffixSorting() = delete;


        [[gnu::always_inline]]
        inline int lcp(int q, int j) {
            q = inverseSuffixArray[q];
            j = inverseSuffixArray[j];

            // don't use recursion in favor of inlining.
            if (q > j) {
                int tmp = q;
                q = j;
                j = tmp;
            }

            int max = INT_MAX;
            while (q++ < j) {
                max = std::min(lcpArray[q], max);
            }
            return max == INT_MAX ? 0 : max;
        }

        [[gnu::hot]]
        inline void compress(Input &input, Output &output) override {
            window = new char[DS_SIZE];
            suffixArray = new int[DS_SIZE];
            inverseSuffixArray = new int[DS_SIZE];
            lcpArray = new int[DS_SIZE];

            StatPhase root("Root"); // causes valgrind problems.

            auto coder = lz77_coder(this->config().sub_config("coder")).encoder(output, NoLiterals());
            coder.factor_length_range(Range(MIN_MATCH, WINDOW_SIZE)); // TODO check if WINDOW_SIZE is enough.
            coder.encode_header();
            tdc::io::InputStream stream = input.as_stream();

            // windowPosition
            uint position = 0;
            // substringPosition
            uint suffixPosition = 0;
            uint lookahead;

            uint maxMatchPos;
            uint maxMatchCount;
            uint currentLcpValue; //current lcp (length) value
            uint currentSuffixValue; // suffixArray value
            uint currentInverseSuffixValue;

            uint inversePositionLower;
            uint inversePositionUpper;

            // isLastBlock equals true, when the stream reached EOF, gcount returned 0 and lookahead > 0
            bool eof = false;

            // initialize window
            stream.read(&window[0], DS_SIZE);
            lookahead = stream.gcount();

            StatPhase::wrap("Init Hashes", [&] {
                initDatastructures(lookahead);
            });

            StatPhase::wrap("Factorize", [&] {
                while (!eof) {
                    while (position < DS_SIZE && lookahead > 0) {
                        assert(lookahead > 0);
                        suffixPosition = position;

                        currentInverseSuffixValue = inverseSuffixArray[suffixPosition];

                        maxMatchCount = 0;
                        maxMatchPos = 0;


                        // process lower part of window[suffixArray[currentInverseSuffixValue]]
                        if (currentInverseSuffixValue > 0) {
                            inversePositionLower = currentInverseSuffixValue - 1;
                            while (window[suffixPosition] == window[suffixArray[inversePositionLower]]) {
                                currentSuffixValue = suffixArray[inversePositionLower];
                                if ((WINDOW_SIZE > suffixPosition ||
                                     suffixPosition - WINDOW_SIZE <= currentSuffixValue) &&
                                    currentSuffixValue < suffixPosition) {
                                    currentLcpValue = lcp(currentSuffixValue, suffixPosition);
                                    if (currentLcpValue > maxMatchCount ||
                                        (currentLcpValue == maxMatchCount && currentSuffixValue > maxMatchPos)) {
                                        maxMatchCount = currentLcpValue;
                                        maxMatchPos = currentSuffixValue;
                                    }
                                    if (currentLcpValue < maxMatchCount) {
                                        break;
                                    }
                                }
                                if (inversePositionLower == 0) {
                                    break;
                                }
                                --inversePositionLower;
                            }
                        }

                        // process upper part of window[suffixArray[currentInverseSuffixValue]]
                        inversePositionUpper = currentInverseSuffixValue + 2;
                        while (inversePositionUpper < DS_SIZE &&
                               window[suffixPosition] == window[suffixArray[inversePositionUpper]]) {
                            currentSuffixValue = suffixArray[inversePositionUpper];
                            if (suffixPosition - WINDOW_SIZE <= currentSuffixValue &&
                                currentSuffixValue < suffixPosition) {
                                currentLcpValue = lcp(currentSuffixValue, suffixPosition);
                                if (currentLcpValue > maxMatchCount ||
                                    (currentLcpValue == maxMatchCount && currentSuffixValue > maxMatchPos)) {
                                    maxMatchCount = currentLcpValue;
                                    maxMatchPos = currentSuffixValue;
                                }
                                if (currentLcpValue < maxMatchCount) {
                                    break;
                                }
                            }
                            ++inversePositionUpper;
                        }

                        // reset
                        if (maxMatchCount == 0) {
                            ++maxMatchCount;
                        }

                        // decide wether we got a literal or a factor.
                        assert(maxMatchCount > 0);
                        if (isLiteral(maxMatchCount)) {
                            addLiteralWord(&window[position], maxMatchCount, coder);

                        } else [[likely]] {
                            addFactor(suffixPosition - maxMatchPos, maxMatchCount, coder);
                        }

                        // reduce lookahead by the number of matched characters.
                        lookahead -= maxMatchCount;
                        position += maxMatchCount;
                    }


                    position = ALPHA * WINDOW_SIZE;
                    if (DS_SIZE > position) {
                        memcpy(&window[0], &window[position], DS_SIZE - position);
                    }
                    uint readBytes = 0;
                    if (stream.good()) { // relevant for last bytes. skip moving memory.
                        stream.read(&window[DS_SIZE - position], position);
                        readBytes = stream.gcount();
                    }

                    if (readBytes > 0) {
                        if (readBytes != ALPHA * WINDOW_SIZE) {
                            std::cout << "check";
                        }
                        lookahead = readBytes;
                        initDatastructures(WINDOW_SIZE + lookahead);
                        position = WINDOW_SIZE;
                    } else {
                        eof = true;
                    }
                }
            });

            #ifdef STATS_ENABLED
            LZ77Helper::printStats(input, root, streampos, factors, WINDOW_SIZE);
            #endif

            delete[] window;
            delete[] suffixArray;
            delete[] inverseSuffixArray;
            delete[] lcpArray;
        }

        [[gnu::always_inline]]
        inline void reconstructTables(uint position, uint bytesInWindow) {
            // initDatastructures(bytesInWindow);
        }

        [[gnu::always_inline]]
        inline void initDatastructures(uint bytesInWindow) {
            // update DS_SIZE, possible scenarios:
            //      -   stream size is smaller than initial DS_SIZE
            //      -   stream size at the end is smaller than ALPHA * WINDOW_SIZE
            DS_SIZE = bytesInWindow;

            // construct arrays
            constructSuffixArray(window, suffixArray, DS_SIZE);
            constructInverseSuffixArray(suffixArray, inverseSuffixArray, DS_SIZE);
            constructLcpArray(window, suffixArray, inverseSuffixArray, lcpArray, DS_SIZE);
        }


        [[gnu::always_inline]]
        inline void addFactor(unsigned int offset, unsigned int length, auto &cod) {
            cod.encode_factor(lzss::Factor(0, offset, length));
            #ifdef STATS_ENABLED
            fac->emplace_back(streampos, offset, length);
            streampos+=length;
            #endif
        }

        [[gnu::always_inline]]
        inline bool isLiteral(uint length) {
            return length < MIN_MATCH;
        }

        [[gnu::always_inline]]
        inline void addLiteralWord(const char *start, uint length, auto &coder) {
            int i = 0;
            for (; i < length; i++) {
                addLiteral(start[i], coder);
            }
        }

        [[gnu::always_inline]]
        inline void
        addLiteral(uliteral_t literal, auto &coder) {
            coder.encode_literal(literal);
            #ifdef STATS_ENABLED
            streampos+=1;
            #endif
        }

        [[gnu::always_inline]]
        inline static void constructSuffixArray(const char *buffer, int *suffixArray, uint size) {
            divsufsort(reinterpret_cast<const sauchar_t *> (buffer), suffixArray, size);
        }

        [[gnu::always_inline]]
        inline static void constructInverseSuffixArray(const int *suffixArray, int *inverseSuffixArray, uint dsSize) {
            for (int i = 0; i < dsSize; i++) {
                inverseSuffixArray[suffixArray[i]] = i;
            }
        }


        [[gnu::always_inline]]
        inline static void constructLcpArray(const char *buffer,
                                             const int *suffixArray,
                                             const int *inverseSuffixArray,
                                             int *lcpArray,
                                             uint dsSize) {
            int h = 0;
            int j;
            lcpArray[0] = 0;
            // calculate height
            for (int i = 0; i < dsSize; i++) {
                if (inverseSuffixArray[i] > 0) {
                    j = suffixArray[inverseSuffixArray[i] - 1];
                    while (std::max(i + h, j + h) < dsSize && buffer[i + h] == buffer[j + h]) {
                        h++;
                    }
                    lcpArray[inverseSuffixArray[i]] = h;
                    if (h > 0) {
                        h--;
                    }
                }
            }
        }

        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder >>();
        }
    };
}
