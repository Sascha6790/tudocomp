#pragma once

//std includes:
#include <vector>
#include <tuple>
#include <unordered_map>

//general includes:
#include <tudocomp/Compressor.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp_stat/StatPhase.hpp>

//sdsl include stree:
//#include <sdsl/suffix_trees.hpp>


#include <tudocomp/ds/BinarySuffixTree.hpp>


//includes encoding:
#include <tudocomp/io/BitIStream.hpp>
#include <tudocomp/io/BitOStream.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/coders/BitCoder.hpp>

#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>


#include <tudocomp/coders/HuffmanCoder.hpp>


namespace tdc {
namespace lfs {

//uint min_lrf = 2,
template<typename literal_coder_t = HuffmanCoder, typename len_coder_t = EliasGammaCoder >
class LFS2BSTCompressor : public Compressor {
private:




    //sdsl::t_csa sa =sdsl::csa_uncompressed<>
    //Suffix Tree type + st
    BinarySuffixTree * stree;

    //Stores nts_symbols of first layer
    IntVector<uint> first_layer_nts;
    // offset to begin of last nts +1. if ==0 no substitution
    IntVector<uint> fl_offsets;
    // stores subs in first layer symbols
    IntVector<uint> second_layer_nts;
    // dead pos in first layer
    BitVector second_layer_dead;

    uint node_count;
    uint max_depth;


    //pair contains begin pos, length
    std::vector<std::pair<uint, uint> > non_terminal_symbols;


    //
    //stores node_ids of corresponding factor length
    std::vector<std::vector<uint> > bins;


    //stores beginning positions corresponding to node_ids
    std::unordered_map< uint , std::vector< uint > >  node_begins;


    typedef sdsl::bp_interval<long unsigned int> node_type;


    inline virtual void compute_string_depth(uint node, uint str_depth){
        //resize if str depth grater than bins size
        uint child = stree->get_first_child(node);


        if(child != 0){
            while(str_depth>= bins.size()){
                bins.resize(bins.size()*2);
            //    std::cerr<<"resizing: "<<bins.size();
            }



            if(str_depth>max_depth){
                max_depth=str_depth;
            }
            node_count++;
            if(str_depth>0){

                bins[str_depth].push_back(node);
            }
            while (child != 0){
                compute_string_depth(child, str_depth + stree->get_edge_length(child));
                child=stree->get_next_sibling(child);
            }

        }

    }




public:

    inline static Meta meta() {
        Meta m("compressor", "lfs2bst",
            "This is an implementation of the longest first substitution compression scheme, type 2.");
        m.needs_sentinel_terminator();
        m.option("min_lrf").dynamic(5);
        m.option("lfs2_lit_coder").templated<literal_coder_t, HuffmanCoder>("lfs2_lit_coder");
        m.option("lfs2_len_coder").templated<len_coder_t, EliasGammaCoder>("lfs2_len_coder");

        return m;
    }


    inline LFS2BSTCompressor(Env&& env):
        Compressor(std::move(env))
    {
        DLOG(INFO) << "Compressor lfs2 instantiated";
    }
    inline virtual void compress(Input& input, Output& output) override {
        uint min_lrf = env().option("min_lrf").as_integer();

        auto in = input.as_view();

        //create vectors:
        first_layer_nts = IntVector<uint>(input.size(), 0);
        fl_offsets = IntVector<uint>(input.size(), 0);
        second_layer_nts = IntVector<uint>(input.size(), 0);
        second_layer_dead = BitVector(input.size(), 0);

     //   std::cerr<<"building stree"<<std::endl;





        StatPhase::wrap("Constructing ST", [&]{
            DLOG(INFO)<<"Constructing ST";
            stree = new BinarySuffixTree(input);
            StatPhase::log("Number of Nodes", stree->get_tree_size());
        });



      //  std::cerr<<"computing lrf"<<std::endl;
        StatPhase::wrap("Computing LRF", [&]{


            StatPhase::wrap("Computing String Depth", [&]{
                bins.resize(200);
                node_count=0;
                max_depth=0;
                compute_string_depth(0,0);

                bins.resize(max_depth +1);
                bins.shrink_to_fit();
            });


           // node_begins.resize(node_counter);

            node_begins.reserve(node_count);

        //    std::cerr<<"iterate st done"<<std::endl;
            uint nts_number = 1 ;
            StatPhase::wrap("Iterate over Node Bins", [&]{
                //iterate node bins top down
                DLOG(INFO)<<"iterate over Node Bins";
                for(uint i = bins.size()-1; i>=min_lrf; i--){

                    //iterate over ids in bin:
                  //  for(auto bin_it = bins[i].begin(); bin_it!= bins[i].end() ; bin_it++){
                    while(!bins[i].empty()){
                        uint no_leaf_id = bins[i].back();
                        bins[i].pop_back();

                        //uint nts_number =0;


                        uint node = no_leaf_id;

                       // DLOG(INFO)<<"Current node: " << node;

                        auto bp = node_begins.find(node);

                        //no begin poss found, get from children

                        if(bp == node_begins.end()){

                         //  DLOG(INFO)<<"Processing bps";

                            std::vector<uint> positions;
                            //get leaves or merge child vectors
                            std::vector<uint> offsets;
                            std::vector<uint> leaf_bps;

                            uint inner = stree->get_first_child(node);
                           // DLOG(INFO)<<"collectint bps:";

                            while (inner != 0){
                              //  DLOG(INFO)<<"addding pos of "<< inner;

                                if(stree->get_first_child(inner) == 0){
                                    //uint temp = stree->get_suffix(inner);


                                    leaf_bps.push_back(stree->get_suffix(inner));

                                } else {
                                    auto & child_bp = node_begins[inner];
                                    if(!child_bp.empty()){
                                        offsets.push_back(positions.size());

                                        positions.insert(positions.end(), child_bp.begin(), child_bp.end());


                                        node_begins.erase(inner);
                                        //(*child_bp).second.clear();

                                    }

                                }
                                inner = stree->get_next_sibling(inner);
                            }
                            //std::sort(positions.begin(), positions.end());
                         //   DLOG(INFO)<<"merging bps:";
                          //  DLOG(INFO)<<"size leaves: "<< leaf_bps.size();

                            std::sort(leaf_bps.begin(), leaf_bps.end());
                            offsets.push_back(positions.size());
                            positions.insert(positions.end(),leaf_bps.begin(), leaf_bps.end());
                            //offsets.push_back(node_begins[no_leaf_id].size());
                            //inplace merge with offset
                          //  DLOG(INFO)<<"offsets: " << offsets.size();
                            for(uint k = 0; k < offsets.size()-1; k++){
                              //  DLOG(INFO)<<"merging from to "<< k << " "<< k+1;
                                std::inplace_merge(positions.begin(), positions.begin()+ offsets[k], positions.begin()+ offsets[k+1]);

                            }
                            //now inplace merge to end
                            std::inplace_merge(positions.begin(), positions.begin()+ offsets.back(), positions.end());

                          //  DLOG(INFO)<<"merging done";


                            node_begins[node]=positions;




                        }

                        std::vector<uint> begin_pos = node_begins[no_leaf_id];
                        //if still empty, because everything is substituted...
                        if(begin_pos.empty()){
                           // bin_it++;
                            continue;
                        }
                        //check if viable lrf, else sort higher!
                        if(begin_pos.size()>=2){

                            if ( ( begin_pos.back() - begin_pos.front() ) >= i ){

                                //greedily iterate over occurences
                                signed long last =  0 - (long) i;
                                std::vector<uint> first_layer_viable;
                                std::vector<uint> second_layer_viable;
                              //  DLOG(INFO)<<"iterating occs, lrf_len: " << i;
                                for(uint occurence : begin_pos){
                                    //check for viability
                                    if( (last+i <= (long) occurence)){
                                        if(fl_offsets[occurence] == 0){
                                       //     DLOG(INFO)<<"first pos of occ ok";
                                            if(fl_offsets[occurence + i -1] == 0){
                                                //Position is firs layer viable
                                        //        DLOG(INFO)<<"occ viable: " << occurence;
                                                first_layer_viable.push_back(occurence);
                                                last= occurence;
                                            }
                                        } else {
                                            //find nts number of symbol that corresponds to substitued occ
                                            uint parent_nts= first_layer_nts[ occurence - (fl_offsets[occurence] -1) ];
                                         //   DLOG(INFO)<<"sl maybe viable: " << occurence << " parent nts: " << parent_nts;
                                            auto nts = non_terminal_symbols[parent_nts-1];
                                            //if length of parent nts is greater than current len + offset
                                       //     DLOG(INFO)<<"offset: "<<fl_offsets[occurence] <<  " len: " << nts.second;
                                            if(nts.second >=fl_offsets[occurence]-1 + i ){
                                                second_layer_viable.push_back(occurence);
                                 //               DLOG(INFO)<<"sl viable, parent length: " << nts.second << ">= " <<fl_offsets[occurence]-1 + i;
                                            }
                                        }

                                    }

                                }


                                //and substitute

                                //if at least 2 first level layer occs viable:
                                // || (first_layer_viable.size() >=1 &&(first_layer_viable.size() + second_layer_viable.size() >= 2))
                                if(first_layer_viable.size() >=1 &&(first_layer_viable.size() + second_layer_viable.size() >= 2) ) {
                                 //   DLOG(INFO)<<"adding new nts";
                                    std::pair<uint,uint> nts = std::make_pair(first_layer_viable.front(), i);
                                    non_terminal_symbols.push_back(nts);

                                    //iterate over vector, make first layer unviable:
                                    for(uint occ : first_layer_viable){
                                      //  DLOG(INFO)<<"fl note nts";
                                        first_layer_nts[occ]= nts_number;

                                      //  DLOG(INFO)<<"fl offset to nts";
                                        for(uint nts_length =0; nts_length < i; nts_length++){
                                            fl_offsets[occ + nts_length] = nts_length+1;
                                        }
                                    }



                                    for(uint sl_occ :second_layer_viable){
                                        uint parent_nts= first_layer_nts[ sl_occ - (fl_offsets[sl_occ] -1) ];
                                        //parten start
                                        auto parent_sym = non_terminal_symbols[parent_nts-1];
                                        uint parent_start= parent_sym.first;
                                        uint sl_start = (parent_start + fl_offsets[sl_occ] -1);
                                        uint sl_end = sl_start+i-1;
                                        if(second_layer_dead[sl_start] == (uint)0 && second_layer_dead[sl_end] == (uint)0){

                                            second_layer_nts[sl_start]=nts_number;

                                            for(uint dead = sl_start; dead<=sl_end;dead++){
                                                second_layer_dead[dead]=1;
                                            }
                                        }
                                    }

                                    //raise nts number:
                                    nts_number++;

                                }



                            }
                        }
                    }

                }

            });

        });

        DLOG(INFO)<<"Computing symbol depth";

        IntVector<uint> nts_depth(non_terminal_symbols.size(), 0);

        for(uint nts_num =0; nts_num<non_terminal_symbols.size(); nts_num++){
            auto symbol = non_terminal_symbols[nts_num];
            uint cur_depth = nts_depth[nts_num];

           // DLOG(INFO)<<"encoding from "<<symbol.first<<" to "<<symbol.second + symbol.first;
            for(uint pos = symbol.first; pos < symbol.second + symbol.first ; pos++){
                if(second_layer_nts[pos]>0){

                    uint symbol_num = second_layer_nts[pos] -1;
                    if(nts_depth[symbol_num]< cur_depth+1){
                        nts_depth[symbol_num]= cur_depth+1;
                    }

                }


            }

        }

        DLOG(INFO)<<"Computing done";

        std::sort(nts_depth.begin(), nts_depth.end());
        if(nts_depth.size()>0){

            uint max_depth = nts_depth[nts_depth.size()-1];

            DLOG(INFO)<<"Max CFG Depth: "<< max_depth;
            DLOG(INFO)<<"Number of CFG rules: "<< non_terminal_symbols.size();

            if(nts_depth.size()>=4){
                uint quarter = nts_depth.size() /4;

           // nts_depth[quarter -1];
                StatPhase::log("25 \% quantil CFG Depth", nts_depth[quarter -1]);
                StatPhase::log("50 \% quantil CFG Depth", nts_depth[(2*quarter) -1]);
                StatPhase::log("75 \% quantil CFG Depth", nts_depth[(3*quarter) -1]);

            }
            StatPhase::log("Max CFG Depth", max_depth);
        }





        StatPhase::log("Number of CFG rules", non_terminal_symbols.size());

        std::stringstream literals;

        for(uint position = 0; position< in.size(); position++){
            if(fl_offsets[position]==0){
                literals << in[position];
            }
        }
        for(uint nts_num = 0; nts_num<=non_terminal_symbols.size(); nts_num++){

            auto symbol = non_terminal_symbols[nts_num];

           // DLOG(INFO)<<"encoding from "<<symbol.first<<" to "<<symbol.second + symbol.first;
            for(uint pos = symbol.first; pos < symbol.second + symbol.first ; pos++){
             //   DLOG(INFO)<<"pos: " <<pos;
                if(second_layer_nts[pos] == 0){
                    literals<< in[pos];

                }
            }
        }



    //    std::cerr<<"encoding text"<<std::endl;

       // std::vector<uint8_t> byte_buffer;

        //Output out_with_buf = output.from_memory(byte_buffer);

        StatPhase::wrap("Encoding Comp", [&]{
            // encode dictionary:
            DLOG(INFO) << "encoding dictionary symbol sizes ";

            std::shared_ptr<BitOStream> bitout = std::make_shared<BitOStream>(output);
            typename literal_coder_t::Encoder lit_coder(
                env().env_for_option("lfs2_lit_coder"),
                bitout,
                ViewLiterals(literals.str())
            );
            typename len_coder_t::Encoder len_coder(
                env().env_for_option("lfs2_len_coder"),
                bitout,
                NoLiterals()
            );

            //encode lengths:
            DLOG(INFO)<<"number nts: " << non_terminal_symbols.size();
            Range intrange (0, UINT_MAX);//uint32_r
            //encode first length:
            if(non_terminal_symbols.size()>=1){
                auto symbol = non_terminal_symbols[0];
                uint last_length=symbol.second;
                //Range for encoding nts number
                Range s_length_r (0,last_length);
                len_coder.encode(last_length,intrange);
                //encode delta length  of following symbols
                for(uint nts_num = 1; nts_num < non_terminal_symbols.size(); nts_num++){
                    symbol = non_terminal_symbols[nts_num];
                    len_coder.encode(last_length-symbol.second,s_length_r);
                    last_length=symbol.second;

                }
                //encode last length, to have zero length last
                len_coder.encode(symbol.second,s_length_r);
            }else {
                len_coder.encode(0,intrange);

            }
            Range dict_r(0, non_terminal_symbols.size());


            long buf_size = bitout->tellp();

           // long alt_size = ;
            StatPhase::log("Bytes Length Encoding", buf_size);
           DLOG(INFO)<<"Bytes Length Encoding: "<< buf_size;
           // DLOG(INFO)<<"Bytes Length Encoding: "<< buf_size;


            DLOG(INFO) << "encoding dictionary symbols";
            uint dict_literals=0;

            // encode dictionary strings, backwards, to directly decode strings:
            if(non_terminal_symbols.size()>=1){
                std::pair<uint,uint> symbol;
                for(long nts_num =non_terminal_symbols.size()-1; nts_num >= 0; nts_num--){
               //     DLOG(INFO)<<"nts_num: " << nts_num;
                    symbol = non_terminal_symbols[nts_num];

                   // DLOG(INFO)<<"encoding from "<<symbol.first<<" to "<<symbol.second + symbol.first;
                    for(uint pos = symbol.first; pos < symbol.second + symbol.first ; pos++){
                     //   DLOG(INFO)<<"pos: " <<pos;
                        if(second_layer_nts[pos] > 0){
                            lit_coder.encode(1, bit_r);
                            lit_coder.encode(second_layer_nts[pos], dict_r);
                            auto symbol = non_terminal_symbols[second_layer_nts[pos] -1];



                            pos += symbol.second - 1;
                        //    DLOG(INFO)<<"new pos "<< pos;

                        } else {
                        //    DLOG(INFO)<<"encoding literal: "<< in[pos];
                            lit_coder.encode(0, bit_r);
                            lit_coder.encode(in[pos],literal_r);
                            dict_literals++;

                        }
                    }
                  //  DLOG(INFO)<<"symbol done";

                }
            }

             uint literals=0;




            buf_size = bitout->tellp() - buf_size;
            StatPhase::log("Bytes Non-Terminal Symbol Encoding", buf_size);


            DLOG(INFO)<<"Bytes Non-Terminal Symbol Encoding: "<< buf_size;

            //encode start symbol

            DLOG(INFO)<<"encode start symbol";
            for(uint pos = 0; pos < in.size(); pos++){
                if(first_layer_nts[pos]>0){
                    lit_coder.encode(1, bit_r);
                    lit_coder.encode(first_layer_nts[pos], dict_r);
                    auto symbol = non_terminal_symbols[first_layer_nts[pos] -1];
                  //  DLOG(INFO)<<"old pos: "<<pos<<" len: " << symbol.second  <<" sl: " << first_layer_nts[pos];

                    pos += symbol.second - 1;
                   // DLOG(INFO)<<"new pos "<< pos;

                } else {
                   // DLOG(INFO)<<"encoding literal: "<< in[pos];
                    lit_coder.encode(0, bit_r);
                    lit_coder.encode(in[pos],literal_r);
                    literals++;


                }
            }

            buf_size = bitout->tellp() - buf_size;
            StatPhase::log("Bytes Start Symbol Encoding", buf_size);


            DLOG(INFO)<<"Bytes Start Symbol Encoding: "<< buf_size;


            StatPhase::log("Literals in Dictionary", dict_literals);

             StatPhase::log("Literals in Start Symbol", literals);
             StatPhase::log("Literals in Input", in.size());

             double literal_percent = ((double)dict_literals + (double)literals)/ (double)in.size();
             StatPhase::log("Literals Encoding / Literals Input", literal_percent);

        //    std::cerr<<"encoding done"<<std::endl;

            DLOG(INFO)<<"encoding done";

        });
    }

    inline virtual void decompress(Input& input, Output& output) override {

        DLOG(INFO) << "decompress lfs";
        std::shared_ptr<BitIStream> bitin = std::make_shared<BitIStream>(input);

        typename literal_coder_t::Decoder lit_decoder(
            env().env_for_option("lfs2_lit_coder"),
            bitin
        );
        typename len_coder_t::Decoder len_decoder(
            env().env_for_option("lfs2_len_coder"),
            bitin
        );
        Range int_r (0,UINT_MAX);

        uint symbol_length = len_decoder.template decode<uint>(int_r);
        Range slength_r (0, symbol_length);
        std::vector<uint> dict_lengths;
        dict_lengths.reserve(symbol_length);
        dict_lengths.push_back(symbol_length);
        while(symbol_length>0){

            uint current_delta = len_decoder.template decode<uint>(slength_r);
            symbol_length-=current_delta;
            dict_lengths.push_back(symbol_length);
        }
        dict_lengths.pop_back();

        DLOG(INFO)<<"decoded number of nts: "<< dict_lengths.size();



        std::vector<std::string> dictionary;
        uint dictionary_size = dict_lengths.size();

        Range dictionary_r (0, dictionary_size);


        dictionary.resize(dict_lengths.size());

        //uint length_of_symbol;
        std::stringstream ss;
        uint symbol_number;
        char c1;
        //c1 = lit_decoder.template decode<char>(literal_r);
       // DLOG(INFO)<<"sync symbol:: "<< c1;

        DLOG(INFO) << "reading dictionary";
        for(long i = dict_lengths.size() -1; i>=0 ;i--){

            ss.str("");
            ss.clear();
            long size_cur = (long) dict_lengths[i];
          //  DLOG(INFO)<<"decoding symbol: "<<i << "length: "  << size_cur;
            while(size_cur > 0){
                bool bit1 = lit_decoder.template decode<bool>(bit_r);

             //   DLOG(INFO)<<"bit indicator: "<<bit1<<" rem len: " << size_cur;

                if(bit1){
                    //bit = 1, is nts, decode nts num and copy
                    symbol_number = lit_decoder.template decode<uint>(dictionary_r); // Dekodiere Literal

              //      DLOG(INFO)<<"read symbol number: "<< symbol_number;
                    symbol_number-=1;

                    if(symbol_number < dictionary.size()){

                        ss << dictionary.at(symbol_number);
                        size_cur-= dict_lengths[symbol_number];
                    } else {
                   //     DLOG(INFO)<< "too large symbol: " << symbol_number;
                        break;
                    }

                } else {
                    //bit = 0, decode literal
                    c1 = lit_decoder.template decode<char>(literal_r); // Dekodiere Literal
                    size_cur--;

                    ss << c1;

                }

            }

            dictionary[i]=ss.str();
          //  DLOG(INFO)<<"add symbol: " << i << " str: "<< ss.str();


        }

        auto ostream = output.as_stream();
        //reading start symbol:
        while(!lit_decoder.eof()){
            //decode bit
            bool bit1 = lit_decoder.template decode<bool>(bit_r);
            char c1;
            uint symbol_number;
            // if bit = 0 its a literal
            if(!bit1){
                c1 = lit_decoder.template decode<char>(literal_r); // Dekodiere Literal

                ostream << c1;
            } else {
            //else its a non-terminal
                symbol_number = lit_decoder.template decode<uint>(dictionary_r); // Dekodiere Literal
                symbol_number-=1;

                if(symbol_number < dictionary.size()){

                    ostream << dictionary.at(symbol_number);
                } else {
                    DLOG(INFO)<< "too large symbol: " << symbol_number;
                }

            }
        }
    }

};

//namespaces closing
}}
