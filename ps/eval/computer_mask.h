#ifndef PS_EVAL_COMPUTER_MASK_H
#define PS_EVAL_COMPUTER_MASK_H

#include "ps/eval/computer.h"
#include <future>
#include "ps/detail/dispatch.h"
#include "ps/eval/evaluator.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"

namespace ps{

namespace mask_computer_detail{

struct rank_hash_eval : evaluator
{
        rank_hash_eval(){
                impl_ = &evaluator_factory::get("6_card_map");
                card_map_7_.resize(rank_hasher::max());

                using iter_t = basic_index_iterator<
                        int, ordered_policy, rank_vector
                >;

                for(iter_t iter(7,13),end;iter!=end;++iter){
                        maybe_add_(*iter);
                }
        }
        ranking_t rank(long a, long b, long c, long d, long e)const override{
                return impl_->rank(a,b,c,d,e);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
                return impl_->rank(a,b,c,d,e,f);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{

                auto shash =  suit_hasher::create_from_cards(a,b,c,d,e,f,g);

                if( suit_hasher::has_flush(shash)){
                        //++miss;
                        return impl_->rank(a,b,c,d,e,f,g);
                }

                auto rhash = rank_hasher::create_from_cards(a,b,c,d,e,f,g);
                auto ret = card_map_7_[rhash];

                return ret;
        }
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const noexcept{


                if( suit_hasher::has_flush_unsafe(suit_hash) ){
                        //return 0; // XXX
                        return impl_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                auto ret = card_map_7_[rank_hash];
                return ret;
        }
private:
        ranking_t rank_from_rank_impl_(long a, long b, long c, long d, long e, long f, long g)const{
                return impl_->rank( card_decl::make_id(0,a),
                                    card_decl::make_id(0,b),
                                    card_decl::make_id(0,c),
                                    card_decl::make_id(0,d),
                                    card_decl::make_id(1,e),
                                    card_decl::make_id(1,f),
                                    card_decl::make_id(1,g) );
        }
        ranking_t rank_from_rank_(long a, long b, long c, long d, long e, long f, long g)const{
                return this->rank( card_decl::make_id(0,a),
                                   card_decl::make_id(0,b),
                                   card_decl::make_id(0,c),
                                   card_decl::make_id(0,d),
                                   card_decl::make_id(1,e),
                                   card_decl::make_id(1,f),
                                   card_decl::make_id(1,g) );
        }
        void maybe_add_(rank_vector const& b){
                // first check we don't have more than 4 of each card
                std::array<int, 13> aux = {0};
                for(size_t i=0;i!=7;++i){
                        ++aux[b[i]];
                }
                for(size_t i=0;i!=aux.size();++i){
                        if( aux[i] > 4 )
                                return;
                }
                auto hash = rank_hasher::create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                auto val  = rank_from_rank_impl_( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                //std::cout << detail::to_string(aux) << " - " << detail::to_string(b) << " => " << std::bitset<30>(static_cast<unsigned long long>(hash)).to_string() << "\n";
                //

                card_map_7_[hash] = val;
        }
        evaluator* impl_;
        std::vector<ranking_t> card_map_7_;
};


} // mask_computer_detail



struct mask_computer : card_eval_computer{
        matrix_t compute_single(computation_context const& ctx, card_eval_instruction const& instr)const noexcept override{
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

                matrix_t mat(ctx.NumPlayers(), ctx.NumPlayers());
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
                return mat * instr.get_matrix();
        }
private:
        mask_computer_detail::rank_hash_eval ev;
        holdem_board_decl w;
};


struct pass_eval_hand_instr : instruction_map_pass{
        //matrix_t compute_single(computation_context const& ctx, card_eval_instruction const& instr)const noexcept override{

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
                tmp.emplace_back(std::make_shared<matrix_instruction>(mat * instr.get_matrix()));
                return tmp;
        }
private:
        mask_computer_detail::rank_hash_eval ev;
        holdem_board_decl w;
};


struct pass_eval_hand_instr_vec : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list){
                using iter_t = decltype(instr_list->begin());
                std::vector<std::tuple<iter_t> > todo;

                for(auto iter(instr_list->begin()),end(instr_list->end());iter!=end;++iter){
                        if( (*iter)->get_type() == instruction::T_CardEval ){
                                todo.emplace_back(iter);
                        }
                }
                
                // short circuit
                if( todo.empty())
                        return;

                std::cout << "todo.size() => " << todo.size() << "\n"; // __CandyPrint__(cxx-print-scalar,todo.size())

                for(auto t : todo){ 
                        auto iter = std::get<0>(t);
                        auto& instr = *reinterpret_cast<card_eval_instruction*>(iter->get());

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
                        *iter = std::make_shared<matrix_instruction>(mat * instr.get_matrix());
                }
        }
private:
        mask_computer_detail::rank_hash_eval ev;
        holdem_board_decl w;
};

} // end namespace ps

#endif // PS_EVAL_COMPUTER_MASK_H
