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
#include <tudocomp/compressors/lzss/LZ77StreamingCoder.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>
#include <tudocomp/coders/EliasDeltaCoder.hpp>
#include "tudocomp/compressors/lz77/LZ77Helper.hpp"
#include "tudocomp/compressors/lz77/ds/CappedWeightedSuffixTree.hpp"

namespace tdc::lz77 {
    using Comparator = bool (*)(WeightedNode<uint> *, uint, uint);

    template<typename lz77_coder>
    class LZ77CompactTries : public Compressor {

    private:

        const size_t minFactorLength;
        const size_t windowSize;


        int i, j, h;
        const int dsSize, bufSize;

        #ifdef STATS_ENABLED
        lzss::FactorBufferRAM factors;
        lzss::FactorBufferRAM *fac;
        std::streampos streamPos = 0;
        #endif
    public:
        class DS {
        public:
            int *suffixArray;
            int *inverseSuffixArray;
            int *lcpArray;
            char *buffer;
            char *first;
            char *second;
            uint gcount;
            int dsSize;
            int windowSize;
            CappedWeightedSuffixTree<unsigned int> *wst;

            DS(int windowSize, const char *bufferW1, std::istream &stream) {
                // dsSize might change (end of stream)
                dsSize = 2 * windowSize;


                buffer = new char[dsSize];
                suffixArray = new int[dsSize];
                inverseSuffixArray = new int[dsSize];
                lcpArray = new int[dsSize];

                memcpy(&this->buffer[0], bufferW1, windowSize);
                stream.read(&buffer[windowSize], windowSize);

                // update dsSize and windowSize on partially read windows (end of stream)
                gcount = stream.gcount();
                dsSize = windowSize + gcount;
                this->windowSize = dsSize / 2;

                first = &this->buffer[0];
                second = &this->buffer[windowSize];


                constructSuffixArray(buffer, suffixArray, dsSize);
                constructInverseSuffixArray(suffixArray, inverseSuffixArray, dsSize);
                constructLcpArray(buffer, suffixArray, inverseSuffixArray, lcpArray, dsSize);

                wst = new CappedWeightedSuffixTree<unsigned int>(lcpArray, suffixArray, buffer, dsSize,
                                                                 this->windowSize);

            }

            virtual ~DS() {
                delete[] suffixArray;
                delete[] inverseSuffixArray;
                delete[] lcpArray;
                delete wst;
                delete[] buffer;
            }


            static void constructInverseSuffixArray(const int *suffixArray, int *inverseSuffixArray, uint dsSize) {
                for (int i = 0; i < dsSize; i++) {
                    inverseSuffixArray[suffixArray[i]] = i;
                }
            }

            static void constructSuffixArray(const char *buffer, int *suffixArray, uint size) {
                divsufsort(reinterpret_cast<const sauchar_t *> (buffer), suffixArray, size);
            }

            static void constructLcpArray(const char *buffer,
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
        };

        inline static Meta meta() {
            Meta m(Compressor::type_desc(), "LZ77CompactTries", "Compute LZ77 Factors using Compact Tries");
            m.param("coder", "The output encoder.").strategy<lz77_coder>(TypeDesc("lzss_coder"));
            m.param("window", "The sliding window size").primitive(16);
            m.param("threshold", "The minimum factor length.").primitive(2);
            m.inherit_tag<lz77_coder>(tags::lossy);
            return m;
        }

        inline LZ77CompactTries() = delete;

        inline explicit LZ77CompactTries(Config &&c) : Compressor(std::move(c)),
                                                       minFactorLength(this->config().param("threshold").as_uint()),
                                                       windowSize(this->config().param("window").as_uint()),
                                                       dsSize(2 * windowSize),
                                                       bufSize(3 * windowSize),
                                                       i(0), j(0), h(0) {
            #ifdef STATS_ENABLED
            fac = &factors;
            #endif
            // Divide Text S into blocks of length w
            // | ---- block 1 ---- | ---- block 2 ---- |
            // Build Patricia Tree where each suffix has a fixed length of w
            // Having a Patricia Tree with a static suffix length requires to build a Patricia Tree with an input of size 2w and truncating each suffix at length w
            // This requires a look ahead of length w. block 1 uses block 2 as look-ahead buffer
            // | ---- block 1 ---- | ---- block 2 ---- | ---- l-ahead ---- |
            // Since we don't want to deal with three buffers, we'll just use one buffer with a size of 3w.
        }

        inline void compress(Input &input, Output &output) override {
            StatPhase root("Root"); // causes valgrind problems.
            // initialize encoder
            auto coder = lz77_coder(this->config().sub_config("coder")).encoder(output, NoLiterals());
            // cod = &coder;
            coder.factor_length_range(Range(minFactorLength, 2 * windowSize));
            coder.encode_header();

            tdc::io::InputStream stream = input.as_stream();

            char *buffer = new char[bufSize];
            unsigned int offset = 0;
            unsigned int blockAMaxLabel = 0;
            unsigned int blockAMatchedChars = 0;
            unsigned int blockBMaxLabel = 0;
            unsigned int blockBMatchedChars = 0;
            bool blockAMore = false;
            bool blockBMore = true;
            bool read = true;
            DS *blockA;
            DS *blockB;
            StatPhase::wrap("Factorize Start", [&] { // caused valgrind problems.
                // handle start of stream
                if (stream.good()) {
                    stream.read(&buffer[0], windowSize);

                    #ifdef STATS_ENABLED
                    streamPos += stream.gcount();
                    #endif

                    StatPhase::wrap("blockA", [&] { // caused valgrind problems.
                        blockA = new DS(windowSize, &buffer[0], stream);
                    });
                    #ifdef STATS_ENABLED
                    streamPos += blockA->gcount;
                    #endif

                    while (offset < blockA->windowSize) {
                        auto node = getEdge(blockA, blockA->buffer, offset, blockAMaxLabel, blockAMatchedChars,
                                            [](WeightedNode<uint> *node, uint index) {
                                                return node->minLabel < index;
                                            });
                        if (blockAMatchedChars > 0) {
                            addFactor(offset - node->minLabel, node->minLabel, blockAMatchedChars, coder);
                            offset += blockAMatchedChars;
                        } else {
                            addLiteral(buffer[offset], coder);
                            ++offset;
                        }

                    }

                    offset -= blockA->windowSize;
                }
            });
            StatPhase::wrap("Factorize Middle", [&] { // caused valgrind problems.
                // handle middle of stream
                int middleSubPhase = 0;
                while (stream.good()) {
                    blockB = new DS(windowSize, blockA->second, stream);
                    #ifdef STATS_ENABLED
                    streamPos += blockB->gcount;
                    #endif
                    while (offset < windowSize) {
                        // optimized call to Comparator by templating, typedef and lambda call
                        auto nodeA = getEdge(blockA, blockB->buffer, offset, blockAMaxLabel, blockAMatchedChars,
                                             [](WeightedNode<uint> *node, uint index) {
                                                 return node->maxLabel >= index;
                                             });
                        auto nodeB = getEdge(blockB, blockB->buffer, offset, blockBMaxLabel, blockBMatchedChars,
                                             [](WeightedNode<uint> *node, uint index) {
                                                 return node->minLabel < index;
                                             });
                        if (blockAMatchedChars <= blockBMatchedChars) {
                            if (blockBMatchedChars == 0) [[unlikely]] {
                                addLiteral(blockA->second[offset], coder);
                                ++offset;
                            } else {
                                // minLabel is within second window => offset - nodeB->minLabel
                                addFactor(offset - nodeB->minLabel, nodeB->minLabel, blockBMatchedChars,
                                          coder);
                                offset += blockBMatchedChars;
                            }
                        } else [[likely]] {
                            // minLabel is within first window => windowSize - nodeA->maxLabel + offset
                            addFactor(windowSize - nodeA->maxLabel + offset, nodeA->maxLabel, blockAMatchedChars,
                                      coder);
                            offset += blockAMatchedChars;
                        }
                    }
                    offset -= windowSize;
                    delete blockA;
                    blockA = blockB;
                }
            });
            StatPhase::wrap("Factorize End", [&] { // caused valgrind problems.
                // handle end of stream
                if (blockA->windowSize != windowSize && blockA->gcount > 0) {
                    offset = 0;
                    // we only parse one block which has exacly gcount chars in blockA->second.
                    // we compare pos < windowSize in getEdge and thus have to set windowSize for the last block to gcount.
                    blockA->windowSize = blockA->gcount;
                    while (offset < blockA->gcount) { // TODO check gcount.
                        auto node = getEdge(blockA, blockA->second, offset, blockAMaxLabel, blockAMatchedChars,
                                            [](WeightedNode<uint> *node, uint index) { return true; });
                        if (blockAMatchedChars > 0) {
                            addFactor(windowSize - node->maxLabel + offset, node->maxLabel, blockAMatchedChars, coder);
                            offset += blockAMatchedChars;
                        } else {
                            addLiteral(blockA->second[offset], coder);
                            ++offset;
                        }

                    }
                }
                // cleanup
                delete[] buffer;
                delete blockA;
            });
            // delete blockB; // not needed because we swap pointers above and delete B there.


            #ifdef STATS_ENABLED
            LZ77Helper::printStats(input, root, streamPos, factors, windowSize);
            #endif
            // stats

        }

        [[gnu::always_inline]]
        inline void
        addLiteral(uliteral_t literal, auto &cod) const {
            cod.encode_literal(literal);
        }

        [[gnu::always_inline]]
        inline void addFactor(unsigned int offset, unsigned int edgeLabel, unsigned int length, auto &cod) const {
            // this->factors.emplace_back(offset, edgeLabel, length);
            cod.encode_factor(lzss::Factor(0, offset, length));
            #ifdef STORE_VECTOR_ENABLED
            fac->emplace_back(0, offset, length);
            #endif
        }

        // modifies maxLabel and matchedChars
        template<class Comparator>
        [[gnu::hot]]
        WeightedNode<unsigned int> *getEdge(const DS *treeStruct,
                                            const char *textToProcess,
                                            unsigned int pos,
                                            unsigned int &maxLabel,
                                            unsigned int &matchedChars,
                                            Comparator comparator) const {
            // reset (might have been set before by another function call)
            maxLabel = 0;
            matchedChars = 0;

            WeightedNode<uint> *currentNode = treeStruct->wst->getRoot();
            // iterate over second block, start with first char of second block.
            unsigned int startPos = pos;
            while (pos < treeStruct->windowSize) {
                // get next node and process matching chars later.
                auto nodeIterator = currentNode->childNodes.find(textToProcess[pos]);
                // check if there is no child starting with the required char.
                if (nodeIterator == currentNode->childNodes.end()) {
                    // node wasn't found
                    return currentNode;
                }
                currentNode = nodeIterator->second;
                if (comparator(currentNode, startPos)) { // label with comparator function.
                    maxLabel = currentNode->maxLabel;
                    // compare edge[0...] with buffer[t..] and increment t by the number of matched chars.
                    for (int edgePos = 0; edgePos < currentNode->edgeLabelLength; edgePos++) {
                        // check if currentPos is too big. (only happens at the end of the stream if the windowSize decreases.)
                        if (treeStruct->windowSize == pos || textToProcess[pos++] != currentNode->edgeLabel[edgePos]) {
                            // edge[0...edgeLength] was not completely matched by buffer[t..]
                            return currentNode;
                        }
                        matchedChars++;
                    }
                } else {
                    // Node is not within the window.
                    return currentNode->parent;
                }
            }
            return currentNode;
        }


        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder>>();
        }
    };
} //ns
