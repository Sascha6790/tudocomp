#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/SparseISA.hpp>

namespace tdc {
    template<typename text_t = TextDS<SADivSufSort, PhiFromSA, PLCPFromPhi, LCPFromPLCP, SparseISA<SADivSufSort>>>
    class LZ77WithoutWindow : public Compressor {
    public:
        inline static Meta meta() {
            Meta m("compressor", "lz77compact", "LZ77 with compact Tries");
            m.option("textds").templated<text_t, TextDS<>>("textds");
            m.uses_textds<text_t>(text_t::SA | text_t::ISA | text_t::LCP);
            return m;
        }

        void compress(Input &input, Output &output) override {
            auto view = input.as_view();
            DCHECK(view.ends_with(uint8_t(0)));

            std::cout << "compress" << std::endl;
            // Construct text data structures
            text_t text = StatPhase::wrap("Construct Text DS", [&]{
                return text_t(env().env_for_option("textds"), view,
                              text_t::SA | text_t::ISA | text_t::LCP);
            });

            auto& sa = text.require_sa();
            auto& isa = text.require_isa();
            auto& lcp = text.require_lcp();

            const len_t text_length = text.size();

            for(int i = 0; i < text_length; i++) {
                std::cout << lcp[i] << std::endl;
            }
        }

        void decompress(Input &input, Output &output) override {

        }

        explicit LZ77WithoutWindow(Env &&env) : Compressor(std::move(env)) {
            std::cout << "LZ77" << std::endl;
        }
    };
}


