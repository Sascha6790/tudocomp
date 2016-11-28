#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/compressors/lz78/TernaryTrie.hpp>

#include <tudocomp/compressors/lzw/LZWDecoding.hpp>
#include <tudocomp/compressors/lzw/LZWFactor.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/coders/BitOptimalCoder.hpp> //default

namespace tdc {

template<typename coder_t, typename dict_t>
class LZWCompressor: public Compressor {
private:
    const lz78::factorid_t m_dict_max_size {0}; //! Maximum dictionary size before reset, 0 == unlimited
public:
    inline LZWCompressor(Env&& env):
        Compressor(std::move(env)),
        m_dict_max_size(env.option("dict_size").as_integer())
    {}

    inline static Meta meta() {
        Meta m("compressor", "lzw", "Lempel-Ziv-Welch\n\n" LZ78_DICT_SIZE_DESC);
        m.option("coder").templated<coder_t, BitOptimalCoder>();
        m.option("lz78trie").templated<dict_t, lz78::TernaryTrie>();
        m.option("dict_size").dynamic("0");
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
		const size_t reserved_size = isqrt(input.size())*2;
        auto is = input.as_stream();

        // Stats
        env().begin_stat_phase("LZW Compression");
        len_t stat_dictionary_resets = 0;
        len_t stat_dict_counter_at_last_reset = 0;
        len_t stat_factor_count = 0;
        len_t factor_count = 0;



        dict_t dict(env().env_for_option("lz78trie"), reserved_size);
		auto reset_dict = [&dict] () {
			dict.clear();
			for(size_t i = 0; i < uliteral_max+1; ++i) {
				const lz78::factorid_t node = dict.add_rootnode(i);
				DCHECK_EQ(node, dict.size());
			}
		};
		reset_dict();
        typename coder_t::Encoder coder(env().env_for_option("coder"), out, NoLiterals());

        lz78::factorid_t node {lz78::undef_id}; // LZ78 node
        char c;
		if(!is.get(c)) return;
		node = static_cast<uliteral_t>(c);

		while(is.get(c)) {
			lz78::factorid_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));
			tdc_debug(VLOG(2) << " child " << child << " #factor " << factor_count << " size " << dict.size() << " node " << node);


			if(child == lz78::undef_id) {
                coder.encode(node, Range(factor_count + uliteral_max + 1));
                stat_factor_count++;
                factor_count++;
				DCHECK_EQ(factor_count+uliteral_max+1, dict.size());
                node = static_cast<uliteral_t>(c); //LZW
				// dictionary's maximum size was reached
				if(dict.size() == m_dict_max_size) { 
					DCHECK_GT(dict.size(),0);
					reset_dict();
					factor_count = 0; //coder.dictionary_reset();
					stat_dictionary_resets++;
					stat_dict_counter_at_last_reset = m_dict_max_size;
				}
			} else { // traverse further
				node = child;
			}
        }

		DLOG(INFO) << "End node id of LZW parsing " << node;
		// take care of left-overs. We do not assume that the stream has a sentinel
		DCHECK_NE(node, lz78::undef_id);
		coder.encode(node, Range(factor_count + uliteral_max + 1)); //LZW
		stat_factor_count++;
		factor_count++;

        env().log_stat("factor_count", stat_factor_count);
        env().log_stat("dictionary_reset_counter", stat_dictionary_resets);
        env().log_stat("max_factor_counter", stat_dict_counter_at_last_reset);
        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {
		const size_t reserved_size = input.size();
        //TODO C::decode(in, out, dms, reserved_size);
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        len_t counter = 0;
		
		//TODO file_corrupted not used!
        lzw::decode_step([&](lz78::factorid_t& entry, bool reset, bool &file_corrupted) -> lzw::Factor {
            if (reset) {
                counter = 0;
            }

            if(decoder.eof()) {
                return false;
            }

            lzw::Factor factor(decoder.template decode<len_t>(Range(counter + uliteral_max + 1)));
            counter++;
            entry = factor;
            return true;
        }, out, m_dict_max_size == 0 ? lz78::DMS_MAX : m_dict_max_size, reserved_size);
    }

};

}

#endif
