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
#include "tudocomp/compressors/lz77/ds/CappedWeightedSuffixTree.hpp"

namespace tdc::lz77 {
    bool maxEdgeComparator(WeightedNode<uint> *node, uint index, uint windowSize) {
        return node->maxLabel >= index - windowSize;
    }

    bool minEdgeComparator(WeightedNode<uint> *node, uint index, uint windowSize) {
        return node->minLabel < index;
    }

    template<typename lzss_coder_t>
    class LZ77CompactTries : public Compressor {

    private:
        const size_t minFactorLength;
        const size_t windowSize;
        lzss::FactorBufferRAM factors;
        int *suffixArrayB1;
        int *suffixArrayB2;
        int *inverseSuffixArrayB1;
        int *inverseSuffixArrayB2;
        int *lcpArrayB1;
        int *lcpArrayB2;
        int i, j, h;
        const int dsSize, bufSize;

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

        inline explicit LZ77CompactTries(Config &&c) : Compressor(std::move(c)),
                                                       minFactorLength(this->config().param("threshold").as_uint()),
                                                       windowSize(this->config().param("window").as_uint()),
                                                       dsSize(2 * windowSize),
                                                       bufSize(3 * windowSize + 1),
                                                       i(0), j(0), h(0) {

            // Divide Text S into blocks of length w
            // | ---- block 1 ---- | ---- block 2 ---- |
            // Build Patricia Tree where each suffix has a fixed length of w
            // Having a Patricia Tree with a static suffix length requires to build a Patricia Tree with an input of size 2w and truncating each suffix at length w
            // This requires a look ahead of length w. block 1 uses block 2 as look-ahead buffer
            // | ---- block 1 ---- | ---- block 2 ---- | ---- l-ahead ---- |
            // Since we don't want to deal with three buffers, we'll just use one buffer with a size of 3w.
            suffixArrayB1 = new int[dsSize];
            suffixArrayB2 = new int[dsSize];
            inverseSuffixArrayB1 = new int[dsSize];
            inverseSuffixArrayB2 = new int[dsSize];
            lcpArrayB1 = new int[dsSize];
            lcpArrayB2 = new int[dsSize];
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
            char *buffer = new char[bufSize];


            while (stream.good()) {
                std::cout << "Stream is good" << std::endl;
                stream.get(&*buffer, bufSize);
                if (stream.eof()) {
                    std::cout << "EOF" << std::endl;
                } else {
                    constructSuffixArray(buffer, suffixArrayB1, dsSize);
                    constructSuffixArray(&buffer[windowSize], suffixArrayB2, dsSize);
                    constructInverseSuffixArray(suffixArrayB1, inverseSuffixArrayB1, dsSize);
                    constructInverseSuffixArray(suffixArrayB2, inverseSuffixArrayB2, dsSize);
                    constructLcpArray(buffer, suffixArrayB1, inverseSuffixArrayB1, lcpArrayB1, dsSize);
                    constructLcpArray(&buffer[windowSize], suffixArrayB2, inverseSuffixArrayB2, lcpArrayB2, dsSize);

                    // TODO reuse stB and swap with stA
                    auto *stA = new CappedWeightedSuffixTree<unsigned int>(lcpArrayB1,
                                                                           suffixArrayB1,
                                                                           buffer,
                                                                           dsSize,
                                                                           windowSize);// Block A
                    auto *stB = new CappedWeightedSuffixTree<unsigned int>(lcpArrayB2,
                                                                           suffixArrayB2,
                                                                           &buffer[windowSize],
                                                                           dsSize,
                                                                           windowSize); // Block B
                    unsigned int offset;
                    int matchedCharsMaxEdge = 0;
                    int matchedCharsMinEdge = 0;
                    for (offset = 0; offset < windowSize; offset++) {
                        // just for debugging, remove later.
                        const char *matchMe = &buffer[windowSize + offset];
                        int maxEBlock1 = getEdge(stA, buffer, offset, matchedCharsMaxEdge, &maxEdgeComparator);
                        int maxEBlock2 = getEdge(stB, buffer, offset, matchedCharsMinEdge, &minEdgeComparator);
                        std::cout << "Factor?: (" << maxEBlock1 << "," << matchedCharsMaxEdge << ")" << std::endl;
                        std::cout << "Factor?: (" << maxEBlock2 << "," << matchedCharsMaxEdge << ")" << std::endl;
                    }
                    debug();
                }
                std::cout << "Number of read chars: " << stream.gcount() << std::endl;
            }
            std::cout << "Stream is not good (anymore)" << std::endl;
            delete[] buffer;
        }

        ~LZ77CompactTries() override {
            // TODO override ? wrong ?
            delete[] suffixArrayB1;
            delete[] suffixArrayB2;
            delete[] inverseSuffixArrayB1;
            delete[] inverseSuffixArrayB2;
            delete[] lcpArrayB1;
            delete[] lcpArrayB2;
        }

        unsigned int getEdge(const CappedWeightedSuffixTree<unsigned int> *stA,
                             const char *buffer,
                             const unsigned &offset,
                             int &matchedChars,
                             bool (*comparator)(WeightedNode<uint> *, uint, uint)) const {
            unsigned int t;
            unsigned int maxE = 0;
            WeightedNode<uint> *currentNode = stA->getRoot();
            WeightedNode<uint> *child = nullptr;

            // just for debugging, remove later.
            const char *matchMe = &buffer[windowSize + offset];
            //use matchedChars to count the number of matched chars.
            int edgePos;
            // iterate over second block, start with first char of second block.
            for (t = windowSize + offset; t < dsSize;) {
                // get next node and process matching chars later.
                auto nodeIterator = currentNode->childNodes.find(buffer[t]);
                // check if there is no child starting with the required char.
                if (nodeIterator == currentNode->childNodes.end()) {
                    // node wasn't found TODO
                    std::cout << buffer[t] << " (" << t << ") not found, breaking" << std::endl;
                    return maxE;
                }

                currentNode = nodeIterator->second;
                if (comparator(currentNode, t, windowSize)) { // check max e within window
                    // might reach end of loop by `t += child->edgeLabelLength` return maxE at the end !  TODO
                    std::cout << "in window " << currentNode->maxLabel << ">=" << t - windowSize << std::endl;
                    maxE = currentNode->maxLabel;

                    // compare char by char and increment t
                    for (edgePos = 0; edgePos < currentNode->edgeLabelLength && t < dsSize; edgePos++) {
                        if (buffer[t++] != currentNode->edgeLabel[edgePos]) {
                            break;
                        }
                        matchedChars++;
                    }

                } else {
                    // CASE: end of string is not within the window. End parsing here. Return latest maxE.
                    std::cout << "not in window " << currentNode->maxLabel << "<" << t - windowSize << std::endl;
                    std::cout << "max_e: " << currentNode->parent->maxLabel << std::endl;
                    return maxE;
                }
            }
            return maxE;
        }

        static void constructInverseSuffixArray(const int *suffixArray, int *inverseSuffixArray, uint dsSize) {
            for (int i = 0; i < dsSize; i++) {
                inverseSuffixArray[suffixArray[i]] = i;
            }
        }

        void debug() {
            for (i = 0; i < dsSize; i++) {
                std::cout << suffixArrayB1[i] << std::endl;
            }
            std::cout << std::endl;
            for (i = 0; i < dsSize; i++) {
                std::cout << inverseSuffixArrayB1[i] << std::endl;
            }
            std::cout << std::endl;
            for (i = 0; i < dsSize; i++) {
                std::cout << lcpArrayB1[i] << std::endl;
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

        inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lzss_coder_t>>();
        }

    };
} //ns
