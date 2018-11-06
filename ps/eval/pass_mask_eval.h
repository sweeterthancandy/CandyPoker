#ifndef PS_EVAL_COMPUTER_MASK_H
#define PS_EVAL_COMPUTER_MASK_H

#include <future>

#include "ps/eval/pass.h"
#include "ps/detail/dispatch.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"

#include "ps/eval/evaluator_6_card_map.h"

#include <unordered_set>
#include <unordered_map>

namespace ps{

namespace mask_computer_detail{

struct rank_hash_eval
{
        rank_hash_eval(){
                card_map_7_.resize(rank_hasher::max());

                using iter_t = basic_index_iterator<
                        int, ordered_policy, rank_vector
                >;

                for(iter_t iter(7,13),end;iter!=end;++iter){
                        //maybe_add_(*iter);
                        auto const& b = *iter;
                        // first check we don't have more than 4 of each card
                        std::array<int, 13> aux = {0};
                        for(size_t i=0;i!=7;++i){
                                ++aux[b[i]];
                        }
                        bool is_possible = [&](){
                                for(size_t i=0;i!=aux.size();++i){
                                        if( aux[i] > 4 )
                                                return true;
                                }
                                return false;
                        }();
                        if( is_possible )
                                continue;

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
        }
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const noexcept{


                if( suit_hasher::has_flush_unsafe(suit_hash) ){
                        return e6cm_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                auto ret = card_map_7_[rank_hash];
                return ret;
        }
private:
        //evaluator_5_card_map* e6cm_{evaluator_5_card_map::instance()};
        evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
        std::vector<ranking_t> card_map_7_;
};


} // mask_computer_detail





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
                tmp.emplace_back(std::make_shared<matrix_instruction>(instrr->group(), mat * instr.get_matrix()));
                return tmp;
        }
private:
        mask_computer_detail::rank_hash_eval ev;
        holdem_board_decl w;
};


namespace pass_eval_hand_instr_vec_detail{

        struct sub_eval{
                using iter_t = instruction_list::iterator;
                sub_eval(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        n = hv.size();
                        mat.resize(n, n);
                        mat.fill(0);
                }
                void accept(size_t mask, std::vector<ranking_t> const& R)noexcept
                {
                        bool cond = (mask & hv_mask ) == 0;
                        if(!cond){
                                return;
                        }
                        for(size_t i=0;i!=n;++i){
                                ranked[i] = R[allocation_[i]];
                        }
                        detail::dispatch_ranked_vector_mat(mat, ranked, n);
                }
                void finish(){
                        *iter_ = std::make_shared<matrix_instruction>(instr_->group(), mat * instr_->get_matrix());
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                template<class Alloc>
                void allocate(Alloc const& alloc){
                        for(size_t idx=0;idx!=n;++idx){
                                allocation_[idx] = alloc(hv[idx]);
                        }
                }
        private:
                iter_t iter_;
                card_eval_instruction* instr_;
                std::array<ranking_t, 9> ranked;
                holdem_hand_vector hv;
                size_t hv_mask;
                matrix_t mat;
                size_t n;
                std::array<size_t, 9> allocation_;
        };

        
        struct sub_eval_two{
                using iter_t = instruction_list::iterator;
                sub_eval_two(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);

                        wins_.fill(0);
                        draw2_.fill(0);
                }
                void accept(size_t mask, std::vector<ranking_t> const& R)noexcept
                {
                        bool cond = (mask & hv_mask ) == 0;
                        if(!cond){
                                return;
                        }
                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];

                        if( r0 < r1 ){
                                ++wins_[0];
                        } else if( r1 < r0 ){
                                ++wins_[1];
                        } else{
                                ++draw2_[0];
                                ++draw2_[1];
                        }
                }
                void finish(){
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
                        }
                        *iter_ = std::make_shared<matrix_instruction>(instr_->group(), mat * instr_->get_matrix());
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                template<class Alloc>
                void allocate(Alloc const& alloc){
                        for(size_t idx=0;idx!=n;++idx){
                                allocation_[idx] = alloc(hv[idx]);
                        }
                }
        private:
                iter_t iter_;
                card_eval_instruction* instr_;

                holdem_hand_vector hv;
                size_t hv_mask;
                matrix_t mat;
                size_t n{2};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
        };
        
        struct sub_eval_three{
                using iter_t = instruction_list::iterator;
                sub_eval_three(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);

                        wins_.fill(0);
                        draw2_.fill(0);
                        draw3_.fill(0);
                }
                void accept(size_t mask, std::vector<ranking_t> const& R)noexcept
                {
                        bool cond = (mask & hv_mask ) == 0;
                        if(!cond){
                                return;
                        }
                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];
                        auto r2 = R[allocation_[2]];

                        if( r0 < r1 ){
                                if( r0 < r2 ){
                                        ++wins_[0];
                                } else if( r2 < r0 ){
                                        ++wins_[2];
                                } else {
                                        ++draw2_[0];
                                        ++draw2_[2];
                                }
                        } else if( r1 < r0 ){
                                if( r1 < r2 ){
                                        ++wins_[1];
                                } else if( r2 < r1 ){
                                        ++wins_[2];
                                } else {
                                        ++draw2_[1];
                                        ++draw2_[2];
                                }
                        } else {
                                // ok one of three casese
                                //    1) r0,r1 draw with each other
                                //    2) r2 wins
                                //    3) r0,r1,r2 draw with each other
                                if( r0 < r2 ){
                                        ++draw2_[0];
                                        ++draw2_[1];
                                } else if( r2 < r0 ){
                                        ++wins_[2];
                                } else {
                                        ++draw3_[0];
                                        ++draw3_[1];
                                        ++draw3_[2];
                                }
                        }
                }
                void finish(){
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
                                mat(2, idx) += draw3_[idx];
                        }
                        *iter_ = std::make_shared<matrix_instruction>(instr_->group(), mat * instr_->get_matrix());
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                template<class Alloc>
                void allocate(Alloc const& alloc){
                        for(size_t idx=0;idx!=n;++idx){
                                allocation_[idx] = alloc(hv[idx]);
                        }
                }
        private:
                iter_t iter_;
                card_eval_instruction* instr_;

                holdem_hand_vector hv;
                size_t hv_mask;
                matrix_t mat;
                size_t n{3};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
                std::array<size_t, 9> draw3_;
        };
        
        template<class T>
        struct basic_sub_eval_factory{
                using sub_ptr_type = std::shared_ptr<T>;
                sub_ptr_type operator()(instruction_list::iterator iter, card_eval_instruction* instr)const{
                        return std::make_shared<T>(iter, instr);
                }
        };
}  // end namespace pass_eval_hand_instr_vec_detail

struct rank_opt_item{
        holdem_id hid;
        size_t mask;
        card_id c0;
        rank_id r0;
        suit_id s0;
        card_id c1;
        rank_id r1; 
        suit_id s1;
};
struct rank_opt_device : std::vector<rank_opt_item>{
        template<class Con>
        static rank_opt_device create(Con const& con){
                rank_opt_device result;
                result.resize(con.size());
                rank_opt_item* out = &result[0];
                for(auto hid : con){
                        auto const& hand{holdem_hand_decl::get(hid)};
                        rank_opt_item item{
                                hid,
                                hand.mask(),
                                hand.first().id(),
                                hand.first().rank().id(),
                                hand.first().suit().id(),
                                hand.second().id(),
                                hand.second().rank().id(),
                                hand.second().suit().id()
                        };
                        *out = item;
                        ++out;
                }
                return result;
        }

};


struct pass_eval_hand_instr_vec : computation_pass{


        template<class Factory>
        void transfrom_impl(computation_context* ctx, instruction_list* instr_list, computation_result* result,
                                   std::vector<typename instruction_list::iterator> const& target_list,
                                   Factory const& factory)
        {
                using sub_ptr_type = typename std::decay_t<Factory>::sub_ptr_type;
                std::vector<sub_ptr_type> subs;

                for(auto& target : target_list){
                        auto instr = reinterpret_cast<card_eval_instruction*>((*target).get());
                        subs.push_back( factory(target, instr) );
                }

                

                std::unordered_set<holdem_id> S;
                for(auto& _ : subs){
                        _->declare(S);
                }
                rank_opt_device rod = rank_opt_device::create(S);
                std::unordered_map<holdem_id, size_t> allocation_table;
                for(size_t idx=0;idx!=rod.size();++idx){
                        allocation_table[rod[idx].hid] = idx;
                }
                for(auto& _ : subs){
                        _->allocate( [&](auto hid){ return allocation_table.find(hid)->second; });
                }

                std::vector<ranking_t> R;
                R.resize(rod.size());
                for(auto const& b : w ){


                        auto mask = b.mask();
                        auto rank_proto = b.rank_hash();
                        auto suit_proto = b.suit_hash();
                        

                        card_vector const& cv = b.board();


                        for(size_t idx=0;idx!=rod.size();++idx){
                                auto const& _ = rod[idx];
                                if( _.mask & mask )
                                        continue;
                                auto rank_hash = rank_proto;
                                auto suit_hash = suit_proto;

                                rank_hash = rank_hasher::append(rank_hash, _.r0);
                                rank_hash = rank_hasher::append(rank_hash, _.r1);

                                suit_hash = suit_hasher::append(suit_hash, _.s0 );
                                suit_hash = suit_hasher::append(suit_hash, _.s1 );

                                ranking_t r = ev.rank(cv, suit_hash, rank_hash, _.c0, _.c1);
                                R[idx] = r;
                        }

                        for(auto& _ : subs){
                                _->accept(mask, R);
                        }
                }
                for(auto& _ : subs){
                        _->finish();
                }
        }
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
                using namespace pass_eval_hand_instr_vec_detail;
                std::vector<instruction_list::iterator> to_map;

                for(auto iter(instr_list->begin()),end(instr_list->end());iter!=end;++iter){
                        if( (*iter)->get_type() == instruction::T_CardEval ){
                                to_map.push_back(iter);
                        }
                }
                // short circuit
                if( to_map.empty())
                        return;
                // as an optimization we want to figure out if we have one size
                std::unordered_set<size_t> n_dist;

                for(auto iter : to_map){
                        auto instr = reinterpret_cast<card_eval_instruction*>((*iter).get());
                        n_dist.insert(instr->get_vector().size());
                }


                if(0){}
                else if( n_dist.size() == 1 && *n_dist.begin() == 2 ){
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval_two>{});
                }
                else if( n_dist.size() == 1 && *n_dist.begin() == 3 ){
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval_three>{});
                } 
                else {
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval>{});
                }
        }
private:
        mask_computer_detail::rank_hash_eval ev;
        holdem_board_decl w;
};

} // end namespace ps

#endif // PS_EVAL_COMPUTER_MASK_H
