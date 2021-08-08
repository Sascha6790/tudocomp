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
    class [[maybe_unused]] LZ77DoubleHashing : public Compressor {

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

        uint h1;
        uint h2;
        uint H2;
        uint *head; // head[base + h2] <- h2 = f2(p)
        uint *prev;
        uint *tmp;
        uint *hashw; // b = hashw[h1] : number of characters to calc the secondary hash
        uint *phash; // base = phash[h1]

        #ifdef STATS_ENABLED
        lzss::FactorBufferRAM factors;
        lzss::FactorBufferRAM *fac;
        std::streampos streamPos = 0;
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


        inline explicit LZ77DoubleHashing(Config &&c) : Compressor(std::move(c)),
                                                        HASH_BITS(this->config().param("HASH_BITS").as_uint()),
                                                        MIN_MATCH(this->config().param("MIN_MATCH").as_uint()),
                                                        MAX_MATCH(this->config().param("MAX_MATCH").as_uint()),
                                                        H_SHIFT((HASH_BITS + MIN_MATCH - 1) / MIN_MATCH),
                                                        WINDOW_SIZE(1 << HASH_BITS),
                                                        HASH_TABLE_SIZE(WINDOW_SIZE),
                                                        HASH_MASK(HASH_TABLE_SIZE - 1),
                                                        WMASK(WINDOW_SIZE - 1),
                                                        MIN_LOOKAHEAD(MAX_MATCH + MIN_MATCH + 1),
                                                        MAX_DIST(WINDOW_SIZE - MIN_LOOKAHEAD) {
            #ifdef STATS_ENABLED
            fac = &factors;
            #endif

            assert(MIN_LOOKAHEAD < WINDOW_SIZE);
            window = new char[2 * WINDOW_SIZE];
            head = new unsigned[HASH_TABLE_SIZE];
            tmp = new unsigned[HASH_TABLE_SIZE];
            hashw = new unsigned[HASH_TABLE_SIZE];
            phash = new unsigned[HASH_TABLE_SIZE];
            prev = new unsigned[WINDOW_SIZE];
            // zero mem
            memset(head, 0, sizeof(unsigned) * HASH_TABLE_SIZE);
            memset(prev, 0, sizeof(unsigned) * WINDOW_SIZE);
        }


        inline void compress(Input &input, Output &output) override {
            StatPhase root("Root"); // causes valgrind problems.

            auto coder = lz77_coder(this->config().sub_config("coder")).encoder(output, NoLiterals());
            coder.factor_length_range(Range(MIN_MATCH, MAX_MATCH));
            coder.encode_header();
            tdc::io::InputStream stream = input.as_stream();
            uint strstart = 0;

            // store best results
            uint maxMatchCount = 0;
            uint maxMatchPos = 0;
            uint currentMatchPos;
            uint currentMatchCount;
            uint currentStrPos;
            uint maxLen;
            uint list;
            bool isLastBlock = false;
            // init
            stream.read(&window[0], 2 * WINDOW_SIZE);
            uint lookahead = stream.gcount();
            StatPhase::wrap("Init Hashes", [&] {
                calculateHashes();
            });

            #ifdef STATS_ENABLED
            streamPos += lookahead;
            #endif

            StatPhase::wrap("Factorize", [&] {
                while (lookahead > 0) {
                    while (lookahead > MIN_LOOKAHEAD || (isLastBlock && lookahead > 0)) {
                        nextHash(strstart);
                        // store results for current iteraton
                        maxMatchPos = 0;
                        currentMatchCount = 0;
                        // init iteration variables

                        uint b = hashw[h1];
                        uint i = 0;
                        uint j = 1;
                        uint k;
                        maxLen = std::min(MAX_MATCH, lookahead);

                        if (strstart > 0) {
                            for (k = 0; k <= b; k++) {
                                for (; i < j; i++) {
                                    list = head[phash[h1] + (h2 ^ i)];
                                    if (list != 0 && prev[list & WMASK] != 0) {
                                        currentStrPos = strstart;
                                        currentMatchPos = prev[list & WMASK];
                                        while (currentMatchCount < maxLen &&
                                               window[currentMatchPos++] == window[currentStrPos++]) {
                                            ++currentMatchCount;
                                        }
                                        // the while loop above increases the positions even though the next char might not match. check and decrease if necessary.
                                        if (window[currentMatchPos] != window[currentStrPos]) {
                                            --currentMatchPos;
                                            --currentStrPos;
                                        }
                                        if (currentMatchCount > maxMatchCount) {
                                            maxMatchCount = currentMatchCount;
                                            maxMatchPos = prev[list & WMASK];
                                        }
                                    }
                                }
                                if (maxMatchCount >= maxLen) {
                                    break;
                                }
                                j = j << 2;
                                maxLen = MAX_MATCH + b - k - 1;
                            }
                        }

                        if (!isLiteral(maxMatchCount)) {
                            addFactor(strstart - maxMatchPos, maxMatchCount, coder);
                            lookahead -= maxMatchCount;
                        } else {
                            if (maxMatchCount == 0) {
                                maxMatchCount = 1; // add at least one char as Literal !
                            }
                            addLiteralWord(&window[strstart], maxMatchCount, coder);
                            lookahead -= maxMatchCount;
                        }
                        --maxMatchCount; // don't parse the current strstart position, skip it. only parse the rest of the matching.
                        ++strstart;
                        // calculate skipped head and prev values caused by a matched string.
                        while (maxMatchCount != 0) {
                            nextHash(strstart);
                            ++strstart;
                            --maxMatchCount;
                        }
                        assert(maxMatchCount == 0);
                    }
                    //--------------------------------------
                    // |-------w1--------|---s----w2-------| // s = strstart
                    //--------------------------------------
                    memcpy(&window[0], &window[strstart], 2 * WINDOW_SIZE - strstart);
                    //--------------------------------------
                    // |s----w2-------xxx|---s----w2-------|    // x = old bytes which weren't moved. second window untouched.
                    //--------------------------------------
                    uint readBytes = 0;
                    if (stream.good()) { // relevant for last bytes. skip moving memory.
                        stream.read(&window[2 * WINDOW_SIZE - strstart], strstart);
                        readBytes = stream.gcount();
                        #ifdef STATS_ENABLED
                        streamPos += readBytes;
                        #endif
                    }
                    //--------------------------------------
                    // |s----w2-------yyy|yyyyyyyyyyyyyyyyyy|    // y = new bytes, second window overwritten.
                    //--------------------------------------

                    // shift head and prev
                    // TODO can be zeroed out if strstart == 0
                    reconstructTables(strstart);
                    // update start and lookahead
                    strstart = 0;

                    lookahead += readBytes;
                    if (!stream.good()) {
                        // lookahead < MIN_LOOKAHEAD but no bytes left.
                        if (!(lookahead > MIN_LOOKAHEAD)) {
                            isLastBlock = true;
                        }
                        // lookahead = 0; // temporary exit criteria
                    }
                }
            });

            StatPhase::wrap("Factorize Cleanup", [&] {
                delete[] window;
                delete[] head;
                delete[] prev;
                delete[] hashw;
                delete[] phash;
                delete[] tmp;
            });

        }

        void reconstructTables(uint strstart) {
            // since I am moving the lookahead to &window[0], head and prev will be zero after the next two loops.
            // TODO: optimize to not discard everything.
            uint h = HASH_TABLE_SIZE;
            while (h-- > 0) {
                head[h] = head[h] > strstart ? head[h] - strstart : 0;
            }
            h = WINDOW_SIZE;
            while (h-- > 0) {
                prev[h] = prev[h] > strstart ? prev[h] - strstart : 0;
            }

            calculateHashes();
        }

        [[gnu::always_inline]]
        inline void nextHash(uint strstart) {
            calculateH1(strstart);
            // calculateH2(strstart);
            calculateH2Ex(strstart);
            uint base = phash[h1];
            prev[strstart & WMASK] = head[base + h2] & WMASK;
            head[base + h2] = strstart; // updates h1
        }

        [[gnu::always_inline]]
        inline void calculateHashes() {
            // reset to zeroes.
            memset(tmp, 0, sizeof(unsigned) * HASH_TABLE_SIZE);

            // count h1-hashes by value and store the result in head/tmp.
            uint i;
            for (i = 0; i < WINDOW_SIZE; i++) {
                calculateH1(i);
                ++tmp[h1];
            }
            // calculate phash and hashw from head (h1-hashes)
            uint h = 0;
            uint b;
            for (i = 0; i < WINDOW_SIZE; i++) {
                phash[i] = h;
                if (tmp[i] > 0) {
                    b = (int) ((int) (log2(tmp[i])) / 2);
                    hashw[i] = b;
                    h += pow(2, 2 * b);
                } else {
                    hashw[i] = 0;
                }
            }
        }

        [[gnu::always_inline]] [[nodiscard]]
        inline bool canReadMaximumLength(ulong strstart) const {
            return strstart <= 2 * WINDOW_SIZE - MIN_LOOKAHEAD;
        }

        [[gnu::always_inline]]
        inline uint f3(uint pos) {
            return window[pos] & 3; // TODO replace 3 with MIN_MATCH ??
        }

        /**
         * used to generate H2 for the whole window.
         * H2 requires an update after moving the window.
         */
        [[gnu::always_inline]]
        inline void initH2() {
            H2 = 0;
            for (uint j = MIN_MATCH; j < MIN_MATCH + (int) (HASH_BITS / 2); j++) {
                H2 = ((H2 << 2) + f3(j)) & WMASK;
            }
        }


        // call on every move of the window
        [[gnu::always_inline]]
        inline void updateH2(uint pos) {
            H2 = ((H2 << 2) + f3(pos + WINDOW_SIZE)) & WMASK;
        }


        [[gnu::always_inline]]
        inline void calculateH2(uint pos) {
            h2 = H2 >> 2 * ((int) (HASH_BITS / 2) - hashw[h1]);
        }

        [[gnu::always_inline]]
        inline void calculateH2Ex(uint pos) {
            // uint len = (int) (HASH_BITS / 2) - hashw[h1];
            uint len = hashw[h1];
            h2 = 0;
            for (uint j = pos; j < pos + len; j++) {
                h2 = ((h2 << 2) + f3(j)) & WMASK;
            }

        }

        [[gnu::always_inline]]
        inline void calculateH1(uint pos) {
            h1 = 0;
            for (uint j = 0; j < MIN_MATCH - 1; j++) {
                h1 = ((h1 << H_SHIFT) ^ window[j + pos]) & HASH_MASK;
            }
        }

        [[gnu::always_inline]]
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
        inline void addFactor(unsigned int offset, unsigned int length, auto &cod) const {
            cod.encode_factor(lzss::Factor(0, offset, length));
            #ifdef STORE_VECTOR_ENABLED
            fac->emplace_back(0, offset, length);
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
        addLiteral(uliteral_t literal, auto &coder) const {
            coder.encode_literal(literal);
        }

        inline LZ77DoubleHashing() = delete;

        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder >>();
        }
    };
} //ns
