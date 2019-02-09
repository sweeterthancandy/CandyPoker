/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_EVAL_PASS_EVAL_HAND_INSTR_H
#define PS_EVAL_PASS_EVAL_HAND_INSTR_H

#include <future>

#include "ps/eval/pass.h"
#include "ps/detail/dispatch.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/base/rank_board_combination_iterator.h"

#include "ps/eval/evaluator_6_card_map.h"

#include <unordered_set>
#include <unordered_map>

namespace ps{

namespace mask_computer_detail{

struct rank_hash_eval
{
        rank_hash_eval(){
                card_map_7_.resize(rank_hasher::max()+1);

                for(rank_board_combination_iterator iter(7),end;iter!=end;++iter){
                        auto const& b = *iter;
                        auto hash = rank_hasher::create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                        auto val = e6cm_->rank( card_decl::make_id(0,b[0]),
                                                card_decl::make_id(0,b[1]),
                                                card_decl::make_id(0,b[2]),
                                                card_decl::make_id(0,b[3]),
                                                card_decl::make_id(1,b[4]),
                                                card_decl::make_id(1,b[5]),
                                                card_decl::make_id(1,b[6]) );

                        PS_ASSERT( hash < card_map_7_.size(), "hash = " << hash << ", card_map_7_.size() = " << card_map_7_.size() );
                        card_map_7_[hash] = val;
                }
        }
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const noexcept{


                if( suit_hasher::has_flush_unsafe(suit_hash) ){
                        return e6cm_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                auto ret = card_map_7_[rank_hash];
                return ret;
        }
        ranking_t rank_flush(card_vector const& cv, long a, long b)const noexcept{
                return e6cm_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
        }
        ranking_t rank_no_flush(size_t rank_hash)const noexcept{
                return card_map_7_[rank_hash];
        }
private:
        //evaluator_5_card_map* e6cm_{evaluator_5_card_map::instance()};
        evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
        std::vector<ranking_t> card_map_7_;
};

struct rank_hash_hash_eval
{
        rank_hash_hash_eval(){
                for(rank_board_combination_iterator iter(7),end;iter!=end;++iter){
                        auto const& b = *iter;
                        auto hash = rank_hasher::create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                        auto val = e6cm_->rank( card_decl::make_id(0,b[0]),
                                                card_decl::make_id(0,b[1]),
                                                card_decl::make_id(0,b[2]),
                                                card_decl::make_id(0,b[3]),
                                                card_decl::make_id(1,b[4]),
                                                card_decl::make_id(1,b[5]),
                                                card_decl::make_id(1,b[6]) );

                        card_map_7_[hash] = val;
                }
                size_t bc = card_map_7_.bucket_count();
                std::cout << "bc => " << bc << "\n"; // __CandyPrint__(cxx-print-scalar,bc)
        }
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const noexcept{
                if( suit_hasher::has_flush_unsafe(suit_hash) ){
                        return e6cm_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                return card_map_7_.find(rank_hash)->second;
        }
        #if 0
        ranking_t rank_mask(size_t mask)const noexcept{
                auto iter = mask_map_.find(mask);
                if( iter == mask_map_.end())
                        return 0;
                return iter->second;
        }
        #endif
private:
        evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
        std::unordered_map<rank_hasher::rank_hash_t, ranking_t> card_map_7_;
};


} // mask_computer_detail

struct pass_eval_hand_instr : instruction_map_pass{

        virtual boost::optional<instruction_list> try_map_instruction(computation_context* ctx, instruction* instrr)override{
                if( instrr->get_type() != instruction::T_CardEval ){
                        return boost::none;
                }
                auto& instr = *reinterpret_cast<card_eval_instruction*>(instrr);

                auto const& hv   = instr.get_vector();
                auto hv_mask = hv.mask();
                        
                // put this here

                // cache stuff

                size_t n = hv.size();
                std::array<ranking_t, 9> ranked;
                std::array<card_id, 9> hv_first;
                std::array<card_id, 9> hv_second;
                std::array<rank_id, 9> hv_first_rank;
                std::array<rank_id, 9> hv_second_rank;
                std::array<suit_id, 9> hv_first_suit;
                std::array<suit_id, 9> hv_second_suit;
                        
                for(size_t i=0;i!=hv.size();++i){
                        auto const& hand{holdem_hand_decl::get(hv[i])};

                        hv_first[i]       = hand.first().id();
                        hv_first_rank[i]  = hand.first().rank().id();
                        hv_first_suit[i]  = hand.first().suit().id();
                        hv_second[i]      = hand.second().id();
                        hv_second_rank[i] = hand.second().rank().id();
                        hv_second_suit[i] = hand.second().suit().id();
                }

                matrix_t mat(ctx->NumPlayers(), ctx->NumPlayers());
                mat.fill(0ull);
                for(auto const& b : w ){

                        bool cond = (b.mask() & hv_mask ) == 0;
                        if(!cond){
                                continue;
                        }
                        auto rank_proto = b.rank_hash();
                        auto suit_proto = b.suit_hash();


                        for(size_t i=0;i!=n;++i){

                                auto rank_hash = rank_proto;
                                auto suit_hash = suit_proto;

                                rank_hash = rank_hasher::append(rank_hash, hv_first_rank[i]);
                                rank_hash = rank_hasher::append(rank_hash, hv_second_rank[i]);

                                suit_hash = suit_hasher::append(suit_hash, hv_first_suit[i] );
                                suit_hash = suit_hasher::append(suit_hash, hv_second_suit[i] );


                                //ranked[i] = 0; continue; // XXX

                                ranked[i] = ev.rank(b.board(), suit_hash, rank_hash, hv_first[i], hv_second[i]);
                        }
                        detail::dispatch_ranked_vector_mat(mat, ranked, n);
                }
                instruction_list tmp;
                tmp.emplace_back(std::make_shared<matrix_instruction>(instrr->group(), mat * instr.get_matrix()));
                return tmp;
        }
private:
        mask_computer_detail::rank_hash_eval ev;
        holdem_board_decl w;
};

} // end namespace ps

#endif // PS_EVAL_PASS_EVAL_HAND_INSTR_H
