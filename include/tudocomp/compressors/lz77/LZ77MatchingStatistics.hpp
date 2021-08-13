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
#include "tudocomp/compressors/lz77/ds/MatchingStatistics.hpp"
#include "tudocomp/compressors/lz77/ds/RmqTree.hpp"
#include "tudocomp/compressors/lz77/ds/CappedWeightedSuffixTree.hpp"

namespace tdc::lz77 {


    struct bitmap {
        bitmap(int n_) {
            n = (n_ + 7) / 8;
            bits = new char[n];
        }

        inline int get(int i) {
            return bits[i >> 3] & (1 << (i & 7));
        }

        inline void set(int i) {
            bits[i >> 3] |= (1 << (i & 7));
        }

        ~bitmap() {
            delete[] bits;
        }

        int n;
        char *bits;
    };

    template<typename lz77_coder>
    class [[maybe_unused]] LZ77MatchingStatistics : public Compressor {

    private:
        const uint HASH_BITS; // dSize: pow(2,HASH_BITS)
        const uint WINDOW_SIZE; // default: 2^15     size of dictionary

        char *window;
        int *suffixArray;
        int *inverseSuffixArray;
        int *lcpArray;

        const uint SKIP_THRESHOLD = 40;

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


        inline explicit LZ77MatchingStatistics(Config &&c) : Compressor(std::move(c)),
                                                             HASH_BITS(this->config().param("HASH_BITS").as_uint()),
                                                             WINDOW_SIZE(1 << HASH_BITS),
                                                             MIN_LOOKAHEAD(WINDOW_SIZE) {
            #ifdef STATS_ENABLED
            fac = &factors;
            #endif

            window = new char[WINDOW_SIZE];

        }

        inline LZ77MatchingStatistics() = delete;


        [[gnu::hot]]
        inline void compress(Input &input, Output &output) override {
            StatPhase root("Root"); // causes valgrind problems.

            auto coder = lz77_coder(this->config().sub_config("coder")).encoder(output, NoLiterals());
            coder.factor_length_range(Range(MIN_MATCH, WINDOW_SIZE)); // TODO check if WINDOW_SIZE is enough.
            coder.encode_header();
            tdc::io::InputStream stream = input.as_stream();

            // windowPosition
            uint position = 0;
            uint lookahead = 0;

            // isLastBlock equals true, when the stream reached EOF, gcount returned 0 and lookahead > 0
            bool eof = false;

            // initialize window
            while (stream.good()) {
                stream.read(&window[0], WINDOW_SIZE);
                lookahead = stream.gcount();
                parse(lookahead, coder);
            }


            #ifdef STATS_ENABLED
            LZ77Helper::printStats(input, root, streampos, factors, WINDOW_SIZE);
            #endif

            //delete here
            delete[] window;
        }

        [[gnu::always_inline]]
        inline void parse(uint fileSize, auto &coder) {
            int *ms_len = new int[WINDOW_SIZE];
            int *ms_pos = new int[WINDOW_SIZE];
            auto *phrase_boundary = new bitmap(fileSize); // Marks ends of phrases in text.
            uint parsed = 0; // Factorized prefix length.
            while (parsed < fileSize) {
                int windowSize = std::min(WINDOW_SIZE, fileSize - parsed);
                char *b = &window[parsed]; // Current block = b[0 .. block_size - 1].

                rmq_tree *rmq = nullptr;
                ms_support *index = nullptr;
                int *SA = new int[windowSize];
                divsufsort(reinterpret_cast<const sauchar_t *>(b), SA, windowSize);

                std::fill(ms_len, &ms_len[windowSize], 0);
                if (parsed > 0) {
                    // Compute matching statistics for the current block.
                    index = new ms_support(b, SA, windowSize);

                    // For each i we compute pos, lcp such that x[i..i+lcp) occurs in b,
                    // lcp is maximized and x[i..i+lcp) = b[SA[pos]..SA[pos]+lcp).
                    uint phrase_end = parsed - 1, phrase_start = phrase_end;
                    while (phrase_start > 0 && !phrase_boundary->get(phrase_start - 1))
                        --phrase_start;
                    int pos = index->dollar, lcp = windowSize;
                    for (uint i = parsed - 1; i >= 0; --i) {
                        index->extend_left(pos, lcp, window[i]);
                        if (lcp && ms_len[pos] < lcp) {
                            ms_len[pos] = lcp;
                            ms_pos[pos] = i;
                        }

                        // Skipping trick. First find the candidate for enclosing phrase.
                        if (i < phrase_start) {
                            phrase_end = --phrase_start;
                            while (phrase_start && !phrase_boundary->get(phrase_start - 1))
                                --phrase_start;
                        }

                        // If it is long enough and x[i..i+lcp) falls inside, restart
                        // matching statistics at the beginning of that phrase.
                        if (phrase_end - phrase_start > SKIP_THRESHOLD
                            && i + lcp - 1 <= phrase_end) {
                            i = phrase_start;
                            std::pair<int, int> match_pair = index->longest_prefix(&window[i]);
                            pos = match_pair.first;
                            lcp = match_pair.second;
                            continue;
                        }
                    }

                    // Matching statistics inversion.
                    for (int i = 1; i < windowSize; ++i)
                        if (std::min(ms_len[i - 1], index->LCP[i]) > ms_len[i]) {
                            ms_len[i] = std::min(ms_len[i - 1], index->LCP[i]);
                            ms_pos[i] = ms_pos[i - 1];
                        }
                    for (int i = windowSize - 2; i >= 0; --i)
                        if (std::min(ms_len[i + 1], index->LCP[i + 1]) > ms_len[i]) {
                            ms_len[i] = std::min(ms_len[i + 1], index->LCP[i + 1]);
                            ms_pos[i] = ms_pos[i + 1];
                        }
                }

                // Parse the currect block (taking matching stats into account).
                int *ISA = parsed ? index->LCP : ms_pos; // Space saving.
                for (int i = 0; i < windowSize; ++i) ISA[SA[i]] = i;
                rmq = new rmq_tree(SA, windowSize, 7);
                int i = 0, pos, len;
                while (i < windowSize) {
                    int psv = rmq->psv(ISA[i], i), nsv = rmq->nsv(ISA[i], i);
                    parse_phrase(b, windowSize, i, psv, nsv, pos, len);
                    pos = len ? pos + parsed : b[i];
                    if ((ms_len[ISA[i]] > len && len) || (!len && ms_len[ISA[i]])) {
                        len = ms_len[ISA[i]];
                        pos = ms_pos[ISA[i]];
                    }

                    // Do not add phrase that can overlap block boundary.
                    if (len && i + len == windowSize && parsed + windowSize < fileSize) break;
                    if (len < MIN_MATCH) {
                        addLiteralWord(&window[parsed + i], std::max(1, len), coder);
                    } else {
                        addFactor(i - pos, len, coder);
                    }
                    i += std::max(1, len);
                    phrase_boundary->set(parsed + i - 1);
                }

                if (i < (windowSize + 1) / 2) {
                    // The last phrase was long -- compute it using pattern matching.
                    std::pair<int, int> lcp_pair = maxlcp(window, fileSize, parsed + i);
                    addFactor(lcp_pair.first, lcp_pair.second, coder);
                    i += std::max(1, lcp_pair.second);
                    phrase_boundary->set(parsed + i - 1);
                }
                parsed += i;
                delete rmq;
                delete index;
                delete[] SA;
            }

            delete[] ms_len;
            delete[] ms_pos;
        }

        [[gnu::always_inline]]
        inline void parse_phrase(char *x, int n, int i, int psv, int nsv,
                                 int &pos, int &len) {
            pos = -1;
            len = 0;
            if (nsv != -1 && psv != -1) {
                while (x[psv + len] == x[nsv + len]) ++len;
                if (x[i + len] == x[psv + len]) {
                    ++len;
                    while (x[i + len] == x[psv + len]) ++len;
                    pos = psv;
                } else {
                    while (i + len < n && x[i + len] == x[nsv + len]) ++len;
                    pos = nsv;
                }
            } else if (psv != -1) {
                while (x[psv + len] == x[i + len]) ++len;
                pos = psv;
            } else if (nsv != -1) {
                while (i + len < n && x[nsv + len] == x[i + len]) ++len;
                pos = nsv;
            }
            if (!len) pos = x[i];
        }

        // Update MS-factorization of t[0..n-1) (variables s,p)
        // to MS-factorizatoin of t[0..n)
        void next(char *t, int &n, int &s, int &p) {
            ++n;
            if (n == 1) {
                s = 0;
                p = 1;
                return;
            }
            int i = n - 1, r = (i - s) % p;
            while (i < n) {
                char a = t[s + r], b = t[i];
                if (a > b) {
                    p = i - s + 1;
                    r = 0;
                } else if (a < b) {
                    i -= r;
                    s = i;
                    p = 1;
                    r = 0;
                } else {
                    ++r;
                    if (r == p) r = 0;
                }
                ++i;
            }
        }

        // Given the MS-factorizarion of t[0..n) return the length of its
        // longst border (say, k) and find MS-factorization of P[0..k)
        [[gnu::always_inline]]
        inline int longest_border(char *t, int n, int &s, int &p) {
            if (n > 0 && 3 * p <= n && !memcmp(t, t + p, s)) return n - p;
            int i = n / 3 + 1, j = 0;
            while (i < n) {
                while (i + j < n && t[i + j] == t[j]) next(t, j, s, p);
                if (i + j == n) return j;
                if (j > 0 && 3 * p <= j && !memcmp(t, t + p, s)) { i += p, j -= p; }
                else {
                    i += j / 3 + 1;
                    j = 0;
                }
            }
            return 0;
        }

        // Return (i, len) that maximizes len = lcp(x[i..n], x[beg..n]), i < beg.
        [[gnu::always_inline]]
        inline std::pair<int, int> maxlcp(char *x, int n, int beg) {
            int len = 0, pos = -1, i = 0, j = 0, s, p;
            while (i < beg) {
                while (beg + j < n && x[i + j] == x[beg + j]) next(x + beg, j, s, p);
                if (j > len) {
                    len = j;
                    pos = i;
                }
                int oldj = j;
                j = longest_border(x + beg, j, s, p);
                if (!j) ++i; else i += oldj - j;
            }
            return std::make_pair(pos, len);
        }

        [[gnu::always_inline]]
        inline void addFactor(unsigned int offset, unsigned int length, auto &cod) {
            cod.encode_factor(lzss::Factor(0, offset, length));
            #ifdef STATS_ENABLED
            fac->emplace_back(0, offset, length);
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
            streampos += 1;
        }

        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder >>();
        }
    };
}
