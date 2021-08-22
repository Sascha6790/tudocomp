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

    #define NOT_SET -1

    template<typename lz77_coder>
    class [[maybe_unused]] LZ77DoubleHashing : public Compressor {

    private:
        const uint HASH_BITS; // dSize: pow(2,HASH_BITS)

        const uint WINDOW_SIZE; // default: 2^15     size of dictionary
        const uint HALF_WINDOW;
        const uint HASH_TABLE_SIZE; // default: 2^15

        const uint MIN_MATCH; // default: 3 MIN_MATCH
        const uint MAX_MATCH; //default: 258 MAX_MATCH
        const uint HASH_MASK;
        const uint H_SHIFT;
        const uint WMASK;
        const uint MIN_LOOKAHEAD;
        const uint MAX_DIST;

        int COMPRESSION_MODE; //might change, but it's unlikely, only for edge-cases.

        char *window;

        int h1;
        int h2;
        int H2;
        int *head; // head[base + h2] <- h2 = f2(p)
        int *prev;
        int *tmp;
        int *hashw; // b = hashw[h1] : number of characters to calc the secondary hash
        int *phash; // base = phash[h1]

        lzss::FactorBufferRAM factors;
        lzss::FactorBufferRAM *fac;
        std::streampos streampos = 0;

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


        inline explicit LZ77DoubleHashing(Config &&c) : Compressor(std::move(c)),
                                                        HASH_BITS(this->config().param("HASH_BITS").as_uint()),
                                                        MIN_MATCH(this->config().param("MIN_MATCH").as_uint()),
                                                        MAX_MATCH(4 * std::pow(2, (int) HASH_BITS / 2) + 1),
                                                        H_SHIFT((HASH_BITS + MIN_MATCH - 1) / MIN_MATCH),
                                                        WINDOW_SIZE(1 << HASH_BITS),
                                                        HALF_WINDOW(WINDOW_SIZE >> 1),
                                                        HASH_TABLE_SIZE(WINDOW_SIZE),
                                                        HASH_MASK(HASH_TABLE_SIZE - 1),
                                                        WMASK(WINDOW_SIZE - 1),
                                                        MIN_LOOKAHEAD(MAX_MATCH + MIN_MATCH + 1),
                                                        COMPRESSION_MODE(
                                                                this->config().param("COMPRESSION_MODE").as_uint()),
                                                        MAX_DIST(WINDOW_SIZE - MIN_LOOKAHEAD) {
            #ifdef FACTORS_ENABLED
            fac = &factors;
            #endif

            // TODO write test.
            if (HASH_BITS > 31) {
                throw new std::invalid_argument("HASH_BITS too big. Maximum allowed value is 31.");
            }

            assert(MIN_LOOKAHEAD < WINDOW_SIZE);


            if (COMPRESSION_MODE != 1) {
                if (WINDOW_SIZE - MIN_LOOKAHEAD <= MIN_LOOKAHEAD) {
                    COMPRESSION_MODE = 1; //default to COMPRESSION_MODE 1 if COMPRESSION_MODE 2 is not available.
                }
            }
        }

        inline LZ77DoubleHashing() = delete;


        [[gnu::hot]]
        inline void compress(Input &input, Output &output) override {

            auto coder = lz77_coder(this->config().sub_config("coder")).encoder(output, NoLiterals());
            coder.factor_length_range(Range(MIN_MATCH, MAX_MATCH));
            coder.encode_header();
            tdc::io::InputStream stream = input.as_stream();

            // isLastBlock equals true, when the stream reached EOF, gcount returned 0 and lookahead > 0

            bool isLastBlock = false;

            uint position = 0;
            uint lookahead;

            int maxMatchPos;
            int numberOfBlocks;
            int listNumber;
            int totalListsToSearch;
            int maxLen;
            int maxMatchCount;
            int currentList;
            int currentMatchPos;
            int currentMatchCount;
            int currentStrPos;

            StatPhase root("Root"); // causes valgrind problems.

            StatPhase::wrap("Init", [&] {
                window = new char[WINDOW_SIZE];
                head = new signed[HASH_TABLE_SIZE];
                tmp = new signed[HASH_TABLE_SIZE];
                hashw = new signed[HASH_TABLE_SIZE];
                phash = new signed[HASH_TABLE_SIZE];
                prev = new signed[WINDOW_SIZE];
                // zero mem
                memset(head, NOT_SET, sizeof(signed) * HASH_TABLE_SIZE);
                memset(prev, NOT_SET, sizeof(signed) * WINDOW_SIZE);

                // initialize window
                stream.read(&window[0], WINDOW_SIZE);
                lookahead = stream.gcount();
                isLastBlock = WINDOW_SIZE != lookahead;

                // calculate and count hashes

                calculateHashes(lookahead);
            });

            StatPhase::wrap("Factorize", [&] {
                while (lookahead > 0) {
                    while (lookahead > MIN_LOOKAHEAD || (isLastBlock && lookahead > 0)) {
                        nextHash(position);

                        // reset
                        maxMatchPos = 0;
                        numberOfBlocks = hashw[h1];
                        listNumber = 0;
                        totalListsToSearch = 1;
                        maxLen = std::min(MAX_MATCH, lookahead);

                        assert(lookahead < 2 * WINDOW_SIZE);
                        maxMatchCount = 1;
                        bool noFullMatch = false;
                        // Loop over every List, start with the "Best" lists, increase the number of lists step by step.
                        for (uint block = 0; block <= numberOfBlocks; block++) {
                            for (; listNumber < totalListsToSearch; listNumber++) {
                                if (phash[h1] == NOT_SET) {
                                    break;
                                }
                                currentList = head[phash[h1] + (h2 ^ listNumber)];
                                if (currentList == NOT_SET) {
                                    continue; // TODO maybe continue ?
                                }
                                currentList &= WMASK;
                                if (prev[currentList] == NOT_SET && currentList == position) {
                                    noFullMatch = true;
                                    continue;
                                }
                                if (noFullMatch || (prev[currentList] == NOT_SET && currentList != position)) {
                                    currentMatchPos = currentList;
                                } else {
                                    currentMatchPos = prev[currentList];
                                }
                                // reset for current list
                                currentStrPos = position;
                                currentMatchCount = 0;
                                uint startOfMatch = currentMatchPos;


                                // trivial comparison of two positions.
                                while (currentMatchCount < maxLen && window[currentMatchPos] == window[currentStrPos]) {
                                    ++currentMatchCount;
                                    ++currentMatchPos;
                                    ++currentStrPos;
                                }

                                // is the current match also the best match so far ?
                                if (currentMatchCount > maxMatchCount) {
                                    maxMatchCount = currentMatchCount;
                                    maxMatchPos = startOfMatch; //TODO +1 removed.
                                    assert(maxMatchPos != NOT_SET);
                                }

                            }
                            if (maxMatchCount >= maxLen) {
                                break;
                            }

                            // Increase number of (searchable) lists by a factor of 4
                            totalListsToSearch <<= 2;
                            maxLen = std::min(lookahead, MIN_MATCH + numberOfBlocks - block - 1);
                        }

                        assert(maxMatchCount > 0);

                        // decide wether we got a literal or a factor.
                        if (isLiteral(maxMatchCount)) {
                            addLiteralWord(&window[position], maxMatchCount, coder);
                        } else [[likely]] {
                            addFactor(position - maxMatchPos, maxMatchCount, coder);
                        }

                        // reduce lookahead by the number of matched characters.
                        assert(lookahead >= maxMatchCount);
                        lookahead -= maxMatchCount;
                        assert(lookahead < 2 * WINDOW_SIZE);

                        // calculate hashes for every position that got matched.
                        // move cursor by the number of matched characters.
                        // skip first character because we already calculated that one.
                        ++position;
                        assert(maxMatchCount > 0);
                        --maxMatchCount;
                        while (maxMatchCount-- != 0) {
                            nextHash(position++);
                        }
                    }

                    uint readBytes = 0;
                    if (COMPRESSION_MODE == 1) {
                        // Strategy 1
                        // copy everything, that is not yet processed to window[0] !
                        // PRO: fast, no need to adjust head and prev tables.
                        // CON: misses a few factors.
                        memcpy(&window[0], &window[position], WINDOW_SIZE - position);

                        // replenish window
                        if (stream.good()) { // relevant for last bytes. skip moving memory.
                            stream.read(&window[WINDOW_SIZE - position], position);
                            readBytes = stream.gcount();
                        }
                        isLastBlock = position != readBytes;
                        lookahead += readBytes;
                        assert(lookahead < 2 * WINDOW_SIZE);
                        // reset tables !
                        memset(head, NOT_SET, sizeof(unsigned) * HASH_TABLE_SIZE);
                        memset(prev, NOT_SET, sizeof(unsigned) * WINDOW_SIZE);
                        calculateHashes(lookahead);
                        position = 0;
                    } else {
                        // Strategy 2
                        // move exactly WINDOW_SIZE bytes
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
                        if (position >= HALF_WINDOW) {
                            memcpy(&window[0], &window[HALF_WINDOW], HALF_WINDOW);

                            if (stream.good()) { // relevant for last bytes. skip moving memory.
                                stream.read(&window[HALF_WINDOW], HALF_WINDOW);
                                readBytes = stream.gcount();
                            }
                            isLastBlock = HALF_WINDOW != readBytes;

                            // uint h = HASH_TABLE_SIZE;
                            // while (h-- > 0) {
                            //     head[h] = head[h] > HALF_WINDOW ? head[h] - HALF_WINDOW : NOT_SET;
                            // }
                            // h = WINDOW_SIZE;
                            // while (h-- > 0) {
                            //     prev[h] = prev[h] > HALF_WINDOW ? prev[h] - HALF_WINDOW : NOT_SET;
                            // }

                            lookahead += readBytes;
                            memset(head, NOT_SET, sizeof(unsigned) * HASH_TABLE_SIZE);
                            memset(prev, NOT_SET, sizeof(unsigned) * WINDOW_SIZE);
                            calculateHashes(std::min(lookahead + HALF_WINDOW, WINDOW_SIZE));

                            position -= HALF_WINDOW;

                            for(int c = 0; c < position; c++) {
                                nextHash(c);
                            }

                        }
                    }
                }
                delete[] window;
                delete[] head;
                delete[] prev;
                delete[] hashw;
                delete[] phash;
                delete[] tmp;
            });

            LZ77Helper::printStats(input, root, streampos, factors, WINDOW_SIZE);
        }

        [[gnu::always_inline]]
        inline void reconstructTables(uint position, uint bytesInWindow) {
            // since I am moving the lookahead to &window[0], head and prev will be zero after the next two loops.
            // TODO: there's room for optimization.
            //  -> don't more strstart to &window[0] but instead to &window[strstart-WINDOW_SIZE]
            uint h = HASH_TABLE_SIZE;
            while (h-- > 0) {
                head[h] = head[h] > position ? head[h] - position : 0;
            }
            h = WINDOW_SIZE;
            while (h-- > 0) {
                prev[h] = prev[h] > position ? prev[h] - position : 0;
            }
        }

        [[gnu::always_inline]]
        inline void nextHash(uint position) {
            calculateH1(position);
            calculateH2Ex(position + MIN_MATCH);
            int base = phash[h1];

            prev[position & WMASK] = head[base + h2];
            if (prev[position & WMASK] != NOT_SET) {
                prev[position & WMASK] &= WMASK;
            }
            head[base + h2] = position; // updates h1
        }

        [[gnu::always_inline]]
        inline void calculateHashes(uint bytesInWindow) {
            bytesInWindow = std::min(bytesInWindow, WINDOW_SIZE);
            // assert(bytesInWindow <= WINDOW_SIZE);
            // reset to zeroes.
            if (bytesInWindow == 0) {
                return;
            }
            memset(tmp, 0, sizeof(unsigned) * HASH_TABLE_SIZE);

            if (bytesInWindow <= MIN_MATCH) {
                return;
            }
            // count h1-hashes by value and store the result in head/tmp.
            uint i;
            for (i = 0; i <= bytesInWindow - MIN_MATCH; i++) {
                calculateH1(i);
                ++tmp[h1];
            }
            // calculate phash and hashw from head (h1-hashes)
            // Remark: paper uses a different approach: keep hashw and reset only on specific conditions.
            int h = 0;
            int b;
            for (i = 0; i < HASH_TABLE_SIZE; i++) {
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

        [[gnu::always_inline]]
        inline uint f3(uint pos) {
            return window[pos] & 3;
        }

        /**
         * used to generate H2 for the whole window.
         * H2 requires an update after moving the window.
         *
         * Remark: NOT WORKING !
         * can't read the whole window because int or event long long don't have enough bytes to
         * represent a number with WINDOW_SIZE bits. However, it's possible to read MIN_MATCH + max(hashw) and adjust accordingly,
         * because h1 gets generated by MIN_MATCH characters and h2 by hashw
         *
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
            for (uint j = 0; j < MIN_MATCH; j++) {
                h1 = ((h1 << H_SHIFT) ^ window[j + pos]) & HASH_MASK;
            }
        }

        [[gnu::always_inline]]
        inline static void constructSuffixArray(const char *buffer, int *suffixArray, uint size) {
            divsufsort(reinterpret_cast<const sauchar_t *> (buffer), suffixArray, size);
        }

        [[gnu::always_inline]]
        inline void addFactor(unsigned int offset, unsigned int length, auto &cod) {
            cod.encode_factor(lzss::Factor(0, offset, length));
            #ifdef FACTORS_ENABLED
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
            #ifdef FACTORS_ENABLED
            streampos += 1;
            #endif
        }

        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder >>();
        }
    };
}
