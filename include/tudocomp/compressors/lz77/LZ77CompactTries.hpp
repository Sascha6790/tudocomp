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
#include "LZ77Helper.hpp"

namespace tdc {
    namespace lz77 {
        template<typename lzss_coder_t>
        class LZ77CompactTries : public Compressor {

        private:
            size_t minFactorLength{};
            size_t slidingWindowSize{};
            lzss::FactorBufferRAM factors;

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

            inline explicit LZ77CompactTries(Config &&c) : Compressor(std::move(c)) {
                minFactorLength = this->config().param("threshold").as_uint();
                slidingWindowSize = this->config().param("window").as_uint();
            }

            inline void compress(Input &input, Output &output) override {
                // initialize encoder
                tdc::lzss::DidacticalCoder::Encoder coder = lzss_coder_t(this->config().sub_config("coder")).encoder(
                        output,
                        NoLiterals());

                coder.factor_length_range(Range(minFactorLength, 2 * slidingWindowSize));
                coder.encode_header();

                tdc::io::InputStream stream = input.as_stream();
                int nextChar;

                RingBuffer<uliteral_t> ahead(slidingWindowSize);
                LZ77Helper::initializeAheadBuffer(stream, nextChar, ahead);

                // factorize
                size_t positionInText = 0; // all symbols before positionInText have already been factorized
                RingBuffer<uliteral_t> window(slidingWindowSize);
            }

            inline std::unique_ptr<Decompressor> decompressor() const override {
                return Algorithm::instance<LZSSDecompressor<lzss_coder_t>>();
            }

        };
    }
} //ns
