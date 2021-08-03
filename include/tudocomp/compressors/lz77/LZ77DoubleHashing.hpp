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

    template<typename lz77_coder>
    class [[maybe_unused]] LZ77DoubleHashing : public Compressor {

    private:

    public:
        inline static Meta meta() {
            Meta m(Compressor::type_desc(), "LZ77CompactTries", "Compute LZ77 Factors using Compact Tries");
            m.param("coder", "The output encoder.").strategy<lz77_coder>(TypeDesc("lz77_coder"));
            m.param("window", "The sliding window size").primitive(16);
            m.param("threshold", "The minimum factor length.").primitive(2);
            m.inherit_tag<lz77_coder>(tags::lossy);
            return m;
        }

        inline LZ77DoubleHashing() = delete;

        inline explicit LZ77DoubleHashing(Config &&c) : Compressor(std::move(c)){
        }

        inline void compress(Input &input, Output &output) override {

        }

        [[nodiscard]] inline std::unique_ptr<Decompressor> decompressor() const override {
            return Algorithm::instance<LZSSDecompressor<lz77_coder>>();
        }
    };
} //ns
