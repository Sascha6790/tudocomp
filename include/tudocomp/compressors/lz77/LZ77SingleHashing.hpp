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
    class [[maybe_unused]] LZ77SingleHashing : public Compressor {

    private:
        const uint HASH_BITS; // dSize: pow(2,HASH_BITS)

        const uint WINDOW_SIZE; // default: 2^15     size of dictionary
        const uint HASH_TABLE_SIZE; // default: 2^15

        const uint MIN_MATCH; // default: 3 MIN_MATCH
        const uint MAX_MATCH; //default: 258 MAX_MATCH
        const uint HASH_MASK;
        const uint H_SHIFT;
        const uint WMASK;
        const uint MIN_LOOKAHEAD;
        const uint MAX_DIST;

        char *window;

        int COMPRESSION_MODE;

        uint hashHead;
        uint *head;
        uint *prev;

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
            m.param("COMPRESSION_MODE", "maximum factor size").primitive(1);
            m.inherit_tag<lz77_coder>(tags::lossy);
            return m;
        }


        inline explicit LZ77SingleHashing(Config &&c) : Compressor(std::move(c)),
                                                        HASH_BITS(this->config().param("HASH_BITS").as_uint()),
                                                        MIN_MATCH(this->config().param("MIN_MATCH").as_uint()),
                                                        MAX_MATCH(std::pow(2, (int) HASH_BITS / 2) + 1),
                                                        H_SHIFT((HASH_BITS + MIN_MATCH - 1) / MIN_MATCH),
                                                        WINDOW_SIZE(1 << HASH_BITS),
                                                        HASH_TABLE_SIZE(WINDOW_SIZE),
                                                        HASH_MASK(HASH_TABLE_SIZE - 1),
                                                        WMASK(WINDOW_SIZE - 1),
                                                        MIN_LOOKAHEAD(MAX_MATCH + MIN_MATCH + 1),
                                                        COMPRESSION_MODE(
                                                                this->config().param("COMPRESSION_MODE").as_uint()),
                                                        MAX_DIST(WINDOW_SIZE - MIN_LOOKAHEAD) {
            assert(MIN_LOOKAHEAD < WINDOW_SIZE);
            window = new char[2 * WINDOW_SIZE];
            head = new unsigned[HASH_TABLE_SIZE];
            prev = new unsigned[WINDOW_SIZE];
            // zero mem
            memset(head, 0, sizeof(unsigned) * HASH_TABLE_SIZE);
            memset(prev, 0, sizeof(unsigned) * WINDOW_SIZE);

            if (COMPRESSION_MODE != 1) {
                if (WINDOW_SIZE - MIN_LOOKAHEAD <= MIN_LOOKAHEAD) {
                    COMPRESSION_MODE = 1; //default to COMPRESSION_MODE 1 if COMPRESSION_MODE 2 is not available.
                }
            }
        }

        inline void compress(Input &input, Output &output) override {
            #ifdef STATS_ENABLED
            fac = &factors;
            #endif

            auto coder = lz77_coder(this->config().sub_config("coder")).encoder(output, NoLiterals());
            coder.factor_length_range(Range(MIN_MATCH, MAX_MATCH));
            coder.encode_header();
            tdc::io::InputStream stream = input.as_stream();
            uint position = 0;

            // store best results
            uint maxMatchCount = 0;
            uint maxMatchPos = 0;
            bool isLastBlock = false;

            StatPhase root("Root"); // causes valgrind problems.
            // StatPhase::wrap("Factorize", [&] { // caused valgrind problems.
            // init
            stream.read(&window[0], 2 * WINDOW_SIZE);
            uint lookahead = stream.gcount();

            while (lookahead > 0) {
                while (lookahead > MIN_LOOKAHEAD || (isLastBlock && lookahead > 0)) {
                    maxMatchPos = 0;

                    f1(position);
                    prev[position & WMASK] = head[hashHead];
                    head[hashHead] = position; // updates h1

                    if (hashHead != 0 && canReadMaximumLength(position)) {
                        // store results for current iteraton
                        uint currentMatchPos;
                        uint currentMatchCount = 0;
                        // init iteration variables
                        uint currentHead = position;
                        uint currentStrPos;
                        uint oldHead = 2 * WINDOW_SIZE;

                        // if currentMatchCount of the previous iteration equals the maximum match -> break loop
                        while (oldHead > currentHead && prev[currentHead & WMASK] != 0 &&
                               currentMatchCount != MAX_MATCH && lookahead > 0) {
                            // init
                            currentStrPos = position;
                            currentMatchCount = 0;
                            currentMatchPos = prev[currentHead & WMASK];

                            // TODO optimize: ignore first 3 chars ?

                            // iterate over chars and match them. (slow ! use double hashing here.)
                            uint maxLen = std::min(MAX_MATCH, lookahead);
                            while (currentMatchCount < maxLen &&
                                   window[currentMatchPos++] == window[currentStrPos++]) {
                                ++currentMatchCount;
                            }
                            // the while loop above increases the positions even though the next char might not match. check and decrease if necessary.
                            if (window[currentMatchPos] != window[currentStrPos]) {
                                --currentMatchPos;
                                --currentStrPos;
                            }
                            // update best match
                            if (currentMatchCount > maxMatchCount) {
                                maxMatchCount = currentMatchCount;
                                maxMatchPos = prev[currentHead & WMASK];
                            }
                            oldHead = currentHead;
                            currentHead = prev[currentHead & WMASK];
                        }
                    }
                    if (!isLiteral(maxMatchCount)) {
                        addFactor(position - maxMatchPos, maxMatchCount, coder);
                        lookahead -= maxMatchCount;
                    } else {
                        if (maxMatchCount == 0) {
                            maxMatchCount = 1; // add at least one char as Literal !
                        }
                        addLiteralWord(&window[position], maxMatchCount, coder);
                        lookahead -= maxMatchCount;
                    }
                    --maxMatchCount; // don't parse the current position, skip it. only parse the rest of the matching.
                    ++position;

                    // calculate skipped head and prev values caused by a matched string.
                    while (maxMatchCount != 0) {
                        f1(position);
                        prev[position & WMASK] = head[hashHead];
                        head[hashHead] = position; // updates h1
                        ++position;
                        --maxMatchCount;
                    }
                    assert(maxMatchCount == 0);

                }
                uint readBytes = 0;
                if (COMPRESSION_MODE == 1) {
                    // Strategy 1
                    // copy everything, that is not yet processed to window[0] !
                    // PRO: fast, no need to adjust head and prev tables.
                    // CON: misses a few factors.
                    memcpy(&window[0], &window[position], 2 * WINDOW_SIZE - position);

                    // replenish window
                    if (stream.good()) { // relevant for last bytes. skip moving memory.
                        stream.read(&window[2 * WINDOW_SIZE - position], position);
                        readBytes = stream.gcount();
                    }

                    // reset tables !
                    memset(head, 0, sizeof(unsigned) * HASH_TABLE_SIZE);
                    memset(prev, 0, sizeof(unsigned) * WINDOW_SIZE);
                    position = 0;
                } else {
                    // Strategy 2
                    // move exactly WINDOW_SIZE bytes
                    // FROM:
                    //--------------------------------------
                    // |-----------------|+++s++++++++++++++| // s = position
                    //--------------------------------------
                    // TO:
                    //--------------------------------------
                    // |+++s+++++++++++++|xxxxxxxxxxxxxxxxxx|    // x has to be replaced by new bytes.
                    //--------------------------------------
                    // Preserve bytes before s and update hashtables accorindly
                    // PRO: can use matches that got calculated before.
                    // CON: slower than just trashing head and prev tables.
                    if (position >= WINDOW_SIZE) {
                        memcpy(&window[0], &window[WINDOW_SIZE], WINDOW_SIZE);

                        if (stream.good()) { // relevant for last bytes. skip moving memory.
                            stream.read(&window[WINDOW_SIZE], WINDOW_SIZE);
                            readBytes = stream.gcount();
                        }

                        uint h = HASH_TABLE_SIZE;
                        while (h-- > 0) {
                            head[h] = head[h] > WINDOW_SIZE ? head[h] - WINDOW_SIZE : 0;
                        }
                        h = WINDOW_SIZE;
                        while (h-- > 0) {
                            prev[h] = prev[h] > WINDOW_SIZE ? prev[h] - WINDOW_SIZE : 0;
                        }

                        position -= WINDOW_SIZE;
                    } else {
                        assert(lookahead == 0); // exit criteria, isLastBlock will be set.
                    }
                }


                lookahead += readBytes;
                if (!stream.good()) {
                    // lookahead < MIN_LOOKAHEAD but no bytes left.
                    if (!(lookahead > MIN_LOOKAHEAD)) {
                        isLastBlock = true;
                    }
                }
            }
            // });
            #ifdef STATS_ENABLED
            LZ77Helper::printStats(input, root, streampos, factors, WINDOW_SIZE);
            #endif

            delete[] window;
            delete[] head;
            delete[] prev;

        }


        [[gnu::always_inline]]
        inline bool canReadMaximumLength(ulong position) const {
            return position <= 2 * WINDOW_SIZE - MIN_LOOKAHEAD;
        }

        inline uint f2(uint pos) {
            return 1;
        }

        inline uint f1(uint pos) {
            hashHead = 0;
            for (uint j = 0; j < MIN_MATCH - 1; j++) {
                hashHead = ((hashHead << H_SHIFT) ^ window[j + pos]) & HASH_MASK;
            }
            return hashHead;
        }

        inline static void constructSuffixArray(const char *buffer, int *suffixArray, uint size) {
            divsufsort(reinterpret_cast<const sauchar_t *> (buffer), suffixArray, size);
        }

        // TODO just example code... don't use a function in real code.
        [[gnu::always_inline]]
        inline void addFactorOrLiteral(const char *buffer, uint pos, uint length, auto &coder) {
            if (isLiteral(length)) {
                addLiteralWord(&buffer[pos], length, coder);
            } else {
                addFactor(pos, length);
            }
        }

        [[gnu::always_inline]]
        inline void addFactor(unsigned int offset, unsigned int length, auto &cod) {
            cod.encode_factor(lzss::Factor(0, offset, length));
            #ifdef STATS_ENABLED
            fac->emplace_back(streampos, offset, length);
            streampos += length;
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
            streampos += 1;
            #endif
        }

        inline LZ77SingleHashing() = delete;

        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder >>();
        }
    };
} //ns
