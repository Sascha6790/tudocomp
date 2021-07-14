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
    template<typename lzss_coder_t>
    class LZ77CompactTries : public Compressor {

    private:
        const size_t minFactorLength;
        const size_t windowSize;
        lzss::FactorBufferRAM factors;
        int *suffixArray;
        int *inverseSuffixArray;
        int *lcpArray;
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
            suffixArray = new int[dsSize];
            inverseSuffixArray = new int[dsSize];
            lcpArray = new int[dsSize];
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
                    //SA-Block-1
                    constructSuffixArray(buffer);
                    constructInverseSuffixArray();
                    constructLcpArray(buffer);
                    auto *stA = new CappedWeightedSuffixTree<unsigned int>(lcpArray, suffixArray, buffer, dsSize, windowSize);// BLock A
                    auto *stB = new CappedWeightedSuffixTree<unsigned int>(lcpArray, suffixArray, &buffer[dsSize], dsSize, windowSize); // Block B

                    int t;
                    WeightedNode<unsigned int> *w1 = stA->getRoot();
                    WeightedNode<unsigned int> *child = nullptr;
                    for (t = windowSize; t < dsSize; t++) {
                        auto node = w1->childNodes.find(buffer[t]);
                        if(node == w1->childNodes.end()) {
                            std::cout << "child";
                            //
                        } else {
                            child = node->second;
                            int max = child->maxLabel;
                            int w = t - windowSize;
                            if(child->maxLabel >= t - windowSize) {
                                std::cout << "in window";
                            }
                            // return node->second;
                        }


                    }

                    WeightedNode<unsigned int> *w2  = stB->getRoot();
                    for (t = windowSize; t < dsSize; t++) {

                    }

                    // traverse Top-Down stA and stB
                    // stA: compare to max(e)
                    //      if max(e) > i - w ? continue : stop
                    // stB: compare to min(e)
                    //      if min(e) < w ? continue: stop
                    // until both stop.
                    // take last edge e of stA and stB
                    // stB : max(e) = start of longest matched string
                    // stB: min(e) = start of longest matched string
                    // add one of them as factor
                    debug();
                }
                std::cout << "Number of read chars: " << stream.gcount() << std::endl;
            }
            std::cout << "Stream id not good (anymore)" << std::endl;
            std::cout << "Cleanup" << std::endl;
            delete[] buffer;
            delete[] suffixArray;
        }

        void constructInverseSuffixArray() {
            for (i = 0; i < dsSize; i++) {
                inverseSuffixArray[suffixArray[i]] = i;
            }
        }

        void debug() {
            for (i = 0; i < dsSize; i++) {
                std::cout << suffixArray[i] << std::endl;
            }
            std::cout << std::endl;
            for (i = 0; i < dsSize; i++) {
                std::cout << inverseSuffixArray[i] << std::endl;
            }
            std::cout << std::endl;
            for (i = 0; i < dsSize; i++) {
                std::cout << lcpArray[i] << std::endl;
            }
        }

        void constructSuffixArray(const char *buffer) const {
            divsufsort(reinterpret_cast<const sauchar_t *> (buffer),
                       suffixArray, dsSize);
        }

        void constructLcpArray(const char *buffer) {
            h = 0;
            lcpArray[0] = 0;
            // calculate height
            for (i = 0; i < dsSize; i++) {
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
