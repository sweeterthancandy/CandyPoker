#include "ps/eval/dispatch_table.h"
#include "ps/eval/generic_shed.h"
#include "ps/eval/generic_sub_eval.h"
#include "ps/eval/optimized_transform.h"

namespace ps{
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
                size_t hand_mask()const{ return hv_mask; }
                void accept_weight(size_t weight, std::vector<ranking_t> const& R)noexcept
                {

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
                size_t n{4};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
                std::array<size_t, 9> draw3_;
                std::array<size_t, 9> draw4_;
        };
        
        struct dispatch_four_player : dispatch_table{
                using transform_type =
                        optimized_transform<
                        sub_eval_four,
                        generic_shed,
                        block_sub_eval_factory<10'000>,
                        rank_hash_eval>;


                virtual bool match(dispatch_context const& dispatch_ctx)const override{
                        return dispatch_ctx.homo_num_players &&  dispatch_ctx.homo_num_players.get() == 4;
                }
                virtual std::shared_ptr<optimized_transform_base> make()const override{
                        return std::make_shared<transform_type>();
                }
                virtual std::string name()const override{
                        return "four-player-generic";
                }
                virtual size_t precedence()const override{ return 100; }
        };
        static register_disptach_table<dispatch_four_player> reg_dispatch_four_player;
} // end namespace ps
