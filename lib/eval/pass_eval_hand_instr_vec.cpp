#include "ps/eval/pass_eval_hand_instr_vec.h"
#include "lib/eval/rank_opt_device.h"
#include "lib/eval/dispatch_table.h"
#include "lib/eval/generic_shed.h"
#include "lib/eval/generic_sub_eval.h"
#include "lib/eval/optimized_transform.h"

namespace ps{




















        
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
                        *iter_ = std::make_shared<matrix_instruction>(instr_->result_desc(), mat);
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
        


        































        template<class SubPtrType>
        struct eval_scheduler_batch_sort{
                // allocation -> group
                struct eval_group{
                        size_t offset;
                        std::vector<size_t> subs;
                };
                struct eval{
                        size_t group;
                        ranking_t rank;
                };
                explicit eval_scheduler_batch_sort(size_t batch_size,
                                               std::vector<SubPtrType>& subs)
                        :batch_size_{batch_size}
                        ,subs_{subs}
                {
                        groups_.resize(batch_size_);

                        size_t count = 0;
                        for(size_t idx=0;idx!=subs_.size();++idx){
                                std::unordered_set<size_t> S;
                                subs_[idx]->declare_allocation(S);
                                for(auto _ : S){
                                        groups_.at(_).subs.push_back(idx);
                                }
                                ++count;
                        }
                        PS_LOG(trace) << "made eval_scheduler_batch_sort";

                        for(size_t idx=0;idx!=groups_.size();++idx){
                                groups_[idx].offset = evals_proto_.size();
                                for(auto _ : groups_[idx].subs ){
                                        evals_proto_.emplace_back(eval{_, 0});
                                }
                        }
                        PS_LOG(trace) << "made eval_scheduler_batch_sort and evals_proto_";
                }

                void begin_eval(mask_set const& ms)noexcept{
                        ms_   = &ms;
                        out_  = 0;
                        evals_ = evals_proto_;
                }
                void put(size_t index, ranking_t rank)noexcept{
                        auto const& G = groups_[index];
                        for(size_t idx=0;idx!=G.subs.size();++idx){
                                evals_.at(G.offset + idx).rank  = rank;
                        }
                }
                void end_eval()noexcept{
                        BOOST_ASSERT( out_ == batch_size_ );

                        /*
                                What I want to do here is optmizize the evaluation.
                                For each subgroup ie,
                                        AsKs TsTc 6h8s,
                                we have to figure out the evaluation, which is one of
                                        {(0), (1), (2), (0,1), (0,2), (1,2), (1,2,3)},
                                which correspond to draws etc.
                                        I'm speculating that this can be optimized,
                                but trying to figure out every batch at once

                         */

                        boost::sort( evals_, [](auto const& l, auto const& r){
                                if( l.rank != r.rank )
                                        return l.rank < r.rank;
                                return l.group < r.group;
                        });

                        #if 0
                        return;
                        std::vector<unsigned> group_A(subs_.size(),0u);
                        std::vector<unsigned> group_B(subs_.size(),0u);
                        std::vector<std::array<unsigned, 3> > group_R(subs_.size());
                        #endif

                        auto emit_level_set = [&](unsigned first, unsigned last){
                                #if 0
                                for(auto iter=first;iter!=evals_.size();++iter){
                                        for(auto group : evals_[iter].groups ){
                                                // set already done
                                                if( group_A[group] != 0 )
                                                        continue;
                                                group_R[group][group_B[group]] = evals_[iter].index;
                                                ++group_B[group];
                                        }
                                }
                                for(auto iter=0;iter!=group_B.size();++iter){
                                        if( group_B[iter] == 0 )
                                                continue;

                                        subs_[iter]->accept_(*ms_, group_B[iter], group_R[iter] );

                                        group_A[iter] += group_B[iter];
                                }
                                #endif
                        };

                        size_t first=0;
                        for(size_t idx=1;idx!=evals_.size();++idx){
                                if( evals_[idx].rank != evals_[first].rank ){
                                        emit_level_set(first, idx);
                                        first = idx;
                                }
                        }
                        emit_level_set(first, evals_.size());
                }
                void regroup()noexcept{
                        // nop
                }
        private:
                size_t batch_size_;
                std::vector<SubPtrType>& subs_;
                mask_set const* ms_;
                size_t out_{0};
                std::vector<eval_group> groups_;
                std::vector<eval> evals_proto_;
                std::vector<eval> evals_;
        };
        

















struct pass_eval_hand_instr_vec_impl{



        pass_eval_hand_instr_vec_impl(std::string const& engine)
                : engine_{engine}
        {}
        virtual void transform_dispatch(computation_context* ctx, instruction_list* instr_list, computation_result* result){

                static optimized_transform_context static_mem_oct;

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

                dispatch_context dctx;
                if( n_dist.size() == 1 )
                        dctx.homo_num_players = *n_dist.begin();

                std::shared_ptr<optimized_transform_base> ot;

                auto construct = [&](auto const& decl){
                        boost::timer::cpu_timer tmr;
                        ot = decl->make();
                        PS_LOG(trace) << "Took " << tmr.format(2, "%w seconds") << " to do make transform";
                        PS_LOG(trace) << "Using transform " << decl->name();
                };

                if( engine_.size() ){
                        // we are looking for a specigfic one
                        for(auto const& item : dispatch_table::world() ){
                                if( item->name() == engine_ ){
                                        construct(item);
                                        break;
                                }
                        }
                        if( ! ot ){
                                PS_LOG(trace) << "======= engines ===========";
                                for(auto const& item : dispatch_table::world() ){
                                        PS_LOG(trace) << "    - " << item->name();
                                }
                        }
                } else {
                        // rules based apprach  
                        for(auto const& item : dispatch_table::world() ){
                                if( item->match(dctx) ){
                                        construct(item);
                                        break;
                                }
                        }
                }
                if( ! ot ){
                        PS_LOG(trace) << "no dispatch";
                } else {
                        boost::timer::cpu_timer tmr;
                        ot->apply(static_mem_oct, ctx, instr_list, result, to_map);
                        PS_LOG(trace) << "Took " << tmr.format(2, "%w seconds") << " to do main loop";
                }
        }
private:
        std::string engine_;
};


pass_eval_hand_instr_vec::pass_eval_hand_instr_vec(std::string const& engine)
        : impl_{std::make_shared<pass_eval_hand_instr_vec_impl>(engine)}
{}
void pass_eval_hand_instr_vec::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        impl_->transform_dispatch(ctx, instr_list, result);
}


} // end namespace ps
