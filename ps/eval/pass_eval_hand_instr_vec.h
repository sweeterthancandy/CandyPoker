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
#ifndef PS_EVAL_COMPUTER_MASK_H
#define PS_EVAL_COMPUTER_MASK_H

#include <future>

#include <boost/timer/timer.hpp>
#include "ps/eval/pass.h"
#include "ps/detail/dispatch.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"

#include "ps/eval/evaluator_6_card_map.h"
#include "ps/eval/pass_eval_hand_instr.h"

#include <unordered_set>
#include <unordered_map>

#include <boost/iterator/counting_iterator.hpp>

namespace ps{



namespace pass_eval_hand_instr_vec_detail{

        /*
                sub_eval represents a single hand vs hand evaluations.
                The idea is that we have a set of evaluations to perform similtanously,
                ie rather than do 
                        for every eval
                                for eval possible board,
                we do
                        for eval possible board
                                for eval eval

                This allows us to do optimizations where cards are shared, for 
                example
                                KcKs sv AhQh
                                KcKs sv AhQd,
                we implement this by three evalutions for each board
                                {board \union KcKs,
                                 board \union AhQh,
                                 board \union AhQd},
                and we save evalutions, we map map
                        board \union KcKs -> integer.


         */
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
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;

                        for(size_t i=0;i!=n;++i){
                                ranked[i] = R[allocation_[i]];
                        }
                        detail::dispatch_ranked_vector_mat(mat, ranked, n, weight);
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
                void accept(size_t mask, size_t weight, std::vector<ranking_t> const& R)noexcept
                {
                        bool cond = (mask & hv_mask ) == 0;
                        if(!cond){
                                return;
                        }
                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];

                        if( r0 < r1 ){
                                wins_[0] += weight;
                        } else if( r1 < r0 ){
                                wins_[1] += weight;
                        } else{
                                draw2_[0] += weight;
                                draw2_[1] += weight;
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
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;

                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];
                        auto r2 = R[allocation_[2]];

                        if( r0 < r1 ){
                                if( r0 < r2 ){
                                        wins_[0] += weight;
                                } else if( r2 < r0 ){
                                        wins_[2] += weight;
                                } else {
                                        draw2_[0] += weight;
                                        draw2_[2] += weight;
                                }
                        } else if( r1 < r0 ){
                                if( r1 < r2 ){
                                        wins_[1] += weight;
                                } else if( r2 < r1 ){
                                        wins_[2] += weight;
                                } else {
                                        draw2_[1] += weight;
                                        draw2_[2] += weight;
                                }
                        } else {
                                // ok one of three casese
                                //    1) r0,r1 draw with each other
                                //    2) r2 wins
                                //    3) r0,r1,r2 draw with each other
                                if( r0 < r2 ){
                                        draw2_[0] += weight;
                                        draw2_[1] += weight;
                                } else if( r2 < r0 ){
                                        wins_[2] += weight;
                                } else {
                                        draw3_[0] += weight;
                                        draw3_[1] += weight;
                                        draw3_[2] += weight;
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


        struct sub_eval_four{
                using iter_t = instruction_list::iterator;
                sub_eval_four(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);

                        wins_.fill(0);
                        draw2_.fill(0);
                        draw3_.fill(0);
                        draw4_.fill(0);
                }
                void accept(size_t mask, size_t weight, std::vector<ranking_t> const& R)noexcept
                {
                        bool cond = (mask & hv_mask ) == 0;
                        if(!cond){
                                return;
                        }
                        auto rank_0 = R[allocation_[0]];
                        auto rank_1 = R[allocation_[1]];
                        auto rank_2 = R[allocation_[2]];
                        auto rank_3 = R[allocation_[3]];

                        // pick best out of 0,1
                        auto l = rank_0;
                        auto li = 0;
                        if( rank_1 < rank_0 ){
                                l = rank_1;
                                li = 1;
                        }

                        // pick best out of 2,3
                        auto r = rank_2;
                        auto ri = 2;
                        if( rank_3 < rank_2 ){
                                r = rank_3;
                                ri = 3;
                        }

                        if( l < r ){
                                if( rank_0 == rank_1 ){
                                        draw2_[0] += weight;
                                        draw2_[1] += weight;
                                } else{
                                        wins_[li] += weight;
                                }
                        } else if( r < l ){
                                if( rank_2 == rank_3 ){
                                        draw2_[2] += weight;
                                        draw2_[3] += weight;
                                } else{
                                        wins_[ri] += weight;
                                }
                        } else {
                                // maybe {0,1,2,3} draw
                                if( rank_0 < rank_1 ){
                                        // {0}, maybe {2,3}
                                        if( rank_2 < rank_3 ){
                                                draw2_[0] += weight;
                                                draw2_[2] += weight;
                                        } else if ( rank_3 < rank_2 ){
                                                draw2_[0] += weight;
                                                draw2_[3] += weight;
                                        } else {
                                                draw3_[0] += weight;
                                                draw3_[2] += weight;
                                                draw3_[3] += weight;
                                        }
                                } else if (rank_1 < rank_0 ){
                                        // {1}, maybe {2,3}
                                        if( rank_2 < rank_3 ){
                                                draw2_[1] += weight;
                                                draw2_[2] += weight;
                                        } else if ( rank_3 < rank_2 ){
                                                draw2_[1] += weight;
                                                draw2_[3] += weight;
                                        } else {
                                                draw3_[1] += weight;
                                                draw3_[2] += weight;
                                                draw3_[3] += weight;
                                        }
                                }
                                else {
                                        // {0,1}, maybe {2,3}
                                        if( rank_2 < rank_3 ){
                                                draw3_[0] += weight;
                                                draw3_[1] += weight;
                                                draw3_[2] += weight;
                                        } else if ( rank_3 < rank_2 ){
                                                draw3_[0] += weight;
                                                draw3_[1] += weight;
                                                draw3_[3] += weight;
                                        } else {
                                                draw4_[0] += weight;
                                                draw4_[1] += weight;
                                                draw4_[2] += weight;
                                                draw4_[3] += weight;
                                        }
                                }
                        }

                }
                void finish(){
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
                                mat(2, idx) += draw3_[idx];
                                mat(3, idx) += draw4_[idx];
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
                size_t n{4};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
                std::array<size_t, 9> draw3_;
                std::array<size_t, 9> draw4_;
        };




        
        template<class T>
        struct basic_sub_eval_factory{
                using sub_ptr_type = std::shared_ptr<T>;
                sub_ptr_type operator()(instruction_list::iterator iter, card_eval_instruction* instr)const{
                        return std::make_shared<T>(iter, instr);
                }
        };


        template<class EvalType, class SubPtrType>
        struct eval_scheduler_simple{
                explicit eval_scheduler_simple(EvalType* impl, size_t batch_size,
                                               std::vector<SubPtrType>& subs)
                        :impl_{impl}
                        ,batch_size_{batch_size}
                        ,subs_{subs}
                {
                        evals_.resize(batch_size_);
                }

                void begin_eval(mask_set const& ms, card_vector const& cv)noexcept{
                        ms_   = &ms;
                        cv_   = &cv;
                        out_  = 0;
                }
                void shedule(size_t index, rank_hasher::rank_hash_t rank_hash)noexcept
                {
                        BOOST_ASSERT( index < batch_size_ );
                        evals_[index] = impl_->rank(rank_hash);
                }
                void shedule_flush(size_t index, rank_hasher::rank_hash_t rank_hash, size_t flush_mask)noexcept
                {
                        BOOST_ASSERT( index < batch_size_ );
                        evals_[index] = impl_->rank_flush(rank_hash, flush_mask);
                }
                void end_eval()noexcept{
                        BOOST_ASSERT( out_ == batch_size_ );
                        for(auto& _ : subs_){
                                _->accept(*ms_, evals_);
                        }
                }
                void regroup()noexcept{
                        // nop
                }
        private:
                EvalType* impl_;
                size_t batch_size_;
                std::vector<ranking_t> evals_;
                std::vector<SubPtrType>& subs_;
                mask_set const* ms_;
                size_t out_{0};
                card_vector const* cv_;
        };
        
        template<class EvalType, class SubPtrType>
        struct eval_scheduler_reshed{
                enum{ DefaultReshedSize = 100 };

                struct atom{
                        size_t group_id;
                        size_t index;
                        rank_hasher::rank_hash_t rank_hash;
                };

                struct group{
                        size_t mask;
                        card_vector const* cv;
                        std::vector<ranking_t> evals;
                };

                explicit eval_scheduler_reshed(EvalType* impl, size_t batch_size,
                                               std::vector<SubPtrType>& subs,
                                               size_t reshed_size = DefaultReshedSize)
                        :impl_{impl}
                        ,batch_size_{batch_size}
                        ,subs_{subs}
                        ,reshed_size_{DefaultReshedSize}
                {
                        size_t alloc_size = reshed_size_ + batch_size_;
                        // we don't need this many gruops, but gyuarenees large enough 
                        groups_.resize(alloc_size);
                        for(auto& g : groups_ ){
                                g.evals.resize(batch_size_);
                        }
                        atoms_.resize(alloc_size);
                        view_proto_.resize(alloc_size);
                        for(size_t idx=0;idx!=alloc_size;++idx){
                                view_proto_[idx] = &atoms_[idx];
                        }
                        view_.reserve(alloc_size);

                }

                void begin_eval(size_t mask, card_vector const& cv)noexcept{
                        groups_[group_iter_].mask = mask;
                        groups_[group_iter_].cv   = &cv;
                }
                void shedule(size_t index, suit_hasher::suit_hash_t suit_hash, rank_hasher::rank_hash_t rank_hash,
                          card_id c0, card_id c1)noexcept
                {
                        bool hash_possible = suit_hasher::has_flush_unsafe(suit_hash); 

                        if( hash_possible ){
                                auto& g = groups_[group_iter_];
                                g.evals[index] = impl_->rank_flush(*g.cv, c0, c1);
                        } else {
                                auto& a = atoms_[atom_iter_];
                                a.group_id  = group_iter_;
                                a.index     = index;
                                a.rank_hash = rank_hash;
                                ++atom_iter_;
                        }
                }
                void end_eval()noexcept{
                        ++group_iter_;
                        if( atom_iter_ >= reshed_size_ ){
                                regroup();
                        }
                }
                void regroup()noexcept{
                        static size_t counter{0};



                        // this is the point of this class, rather than execute each atom in series 
                        // nativly, I can reorder them to take advantage of CPU cache
                        //
                        // execute every atom
                        //

                        view_ = view_proto_;
                        view_.resize(atom_iter_);

                        boost::sort(view_, [](auto const& l, auto const& r){
                                return l->rank_hash < r->rank_hash;
                        });

                        for(auto ptr : view_){
                                auto const& a = *ptr;
                                auto& g = groups_[a.group_id];
                                g.evals[a.index] = impl_->rank_no_flush(a.rank_hash);
                        }
                        
                        for(size_t idx=0;idx<group_iter_;++idx){
                                auto const& g = groups_[idx];
                                for(auto& _ : subs_){
                                        _->accept(g.mask, g.evals);
                                }
                        }
                        atom_iter_ = 0;
                        group_iter_ = 0;

                        ++counter;
                        //std::cout << "counter => " << counter << "\n"; // __CandyPrint__(cxx-print-scalar,counter)
                                 
                }
        private:
                EvalType* impl_;
                size_t batch_size_;
                std::vector<SubPtrType>& subs_;
                size_t reshed_size_;

                std::vector<group> groups_;
                std::vector<atom> atoms_;

                size_t atom_iter_{0};
                size_t group_iter_{0};
                std::vector<atom*> view_proto_;
                std::vector<atom*> view_;
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

        //using eval_type = mask_computer_detail::rank_hash_hash_eval;
        using eval_type = mask_computer_detail::rank_hash_eval;
        //using eval_type =   mask_computer_detail::SKPokerEvalWrap;


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

                PS_LOG(trace) << "Have " << subs.size() << " subs";

                std::vector<ranking_t> R;
                R.resize(rod.size());

                                     
                
                boost::timer::cpu_timer tmr;
                #if 1
                
                using shed_type = pass_eval_hand_instr_vec_detail::eval_scheduler_simple<eval_type, sub_ptr_type>;
                //using shed_type = pass_eval_hand_instr_vec_detail::eval_scheduler_reshed<mask_computer_detail::rank_hash_eval, sub_ptr_type>;
                shed_type shed{&ev, rod.size(), subs};
                for(auto const& weighted_pair : w.weighted_rng() ){
                //for(auto const& b : w ){

                        auto const& b = *weighted_pair.board;

                        auto mask             = b.mask();
                        auto rank_proto       = b.rank_hash();
                        auto suit_proto       = b.suit_hash();
                        card_vector const& cv = b.board();

                        auto flush_possible   = b.flush_possible();
                        suit_id flush_suit    = b.flush_suit();
                        auto const& flush_suit_board = b.flush_suit_board();
                        size_t fsbsz = flush_suit_board.size();
                        auto flush_mask = b.flush_mask();


                        shed.begin_eval(weighted_pair.masks, cv);

                        for(size_t idx=0;idx!=rod.size();++idx){
                                auto const& _ = rod[idx];

                                // this is slower
                                #if 0
                                bool cond = ( weighted_pair.masks.count_disjoint(_.mask) != 0 );
                                if( ! cond )
                                        continue;
                                #endif


                                auto rank_hash = rank_proto;

                                rank_hash = rank_hasher::append(rank_hash, _.r0);
                                rank_hash = rank_hasher::append(rank_hash, _.r1);

                                if( fsbsz == 0 ){
                                        shed.shedule(idx, rank_hash);
                                } else {

                                        auto fm = flush_mask;

                                        bool s0m = ( _.s0 == flush_suit );
                                        bool s1m = ( _.s1 == flush_suit );

                                        if( s0m )
                                                fm |= 1ull << _.r0;
                                        if( s1m )
                                                fm |= 1ull << _.r1;

                                        shed.shedule_flush(idx, rank_hash, fm);
                                }

                        }
                        shed.end_eval();
                }
                shed.regroup();

                #else
                for(auto const& b : w ){


                        auto mask             = b.mask();
                        auto rank_proto       = b.rank_hash();
                        auto suit_proto       = b.suit_hash();
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
                #endif
                
                PS_LOG(trace) << "Took " << tmr.format(2, "%w seconds") << " to do main loop";
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
                #if 0
                else if( n_dist.size() == 1 && *n_dist.begin() == 2 ){
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval_two>{});
                }
                #endif
                else if( n_dist.size() == 1 && *n_dist.begin() == 3 ){
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval_three>{});
                } 
                #if 0
                else if( n_dist.size() == 1 && *n_dist.begin() == 4 ){
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval_four>{});
                } 
                #endif
                else
                {
                        transfrom_impl( ctx, instr_list, result, to_map, basic_sub_eval_factory<sub_eval>{});
                }
        }
private:
        eval_type ev;
        holdem_board_decl w;
};

} // end namespace ps

#endif // PS_EVAL_COMPUTER_MASK_H
