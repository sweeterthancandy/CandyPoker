#ifndef LIB_EVAL_GENERIC_SHED_H
#define LIB_EVAL_GENERIC_SHED_H

#include "ps/eval/optimized_transform.h"

namespace ps{

        //using eval_counter_type = std::uint_fast32_t;
        using eval_counter_type = size_t;

        /*
                This was originally meant to be a small interface, and then
                from the small interface be able to change the order of evaluations,
                thinking I could do something in terns or reordering for CPU-cache
                improvment. This didn't work. but this is still helpfull
         */

        struct generic_weight_policy{
                // this is 5% faster (in my limited testing)
                enum{ CheckUnion = true };
                eval_counter_type calculate(size_t hv_mask, mask_set const* ms, size_t single_mask)const noexcept{
                        if( !! ms ){
                                if constexpr( CheckUnion ){
                                        if( ( ms->get_union() & hv_mask) == 0 ){
                                                return ms->size();
                                        }
                                }
                                return ms->count_disjoint_with<eval_counter_type>(hv_mask);
                        } else {
                                return (( hv_mask & single_mask )==0?1:0);
                        }
                }
                eval_counter_type calculate(size_t hv_mask, mask_set const& ms)const noexcept {
                    if constexpr (CheckUnion) {
                        if ((ms.get_union() & hv_mask) == 0) {
                            return ms.size();
                        }
                    }
                    return ms.count_disjoint_with<eval_counter_type>(hv_mask);
                }
        };

        struct generic_shed{

                using weights_ty = std::vector<eval_counter_type>;
                template<class SubPtrType>
                struct bind{
                        // this is 5% faster (in my limited testing)
                        enum{ CheckZeroWeight = true };
                        explicit bind(holdem_hand_vector const& allocation, std::vector<SubPtrType>& subs)
                                :subs_{subs}
                        {
                        }
                        void put(size_t index, ranking_t rank)noexcept{
                                evals_[index] = rank;
                        }

                        int end_eval(mask_set const* ms, size_t single_mask)noexcept{
                                (void)single_mask;
                                int non_zero{ 0 };
                                for(auto& _ : subs_){
                                        auto weight = generic_weight_policy{}
                                                .calculate(_->hand_mask(), *ms);
                                        if constexpr ( CheckZeroWeight ){
                                                if( weight == 0 )
                                                        continue;
                                        }
                                        _->accept_weight(weight, evals_);
                                        ++non_zero;
                                }
                                return non_zero;
                        }

                        int end_eval_single(size_t mask)noexcept {
                            int non_zero{ 0 };
                            for (auto& _ : subs_) {
                                if ((_->hand_mask() & mask) == 0)
                                {
                                    _->accept_weight(1, evals_);
                                }
                                ++non_zero;
                            }
                            return non_zero;
                        }

                        void regroup()noexcept{
                                // nop
                        }



                        int end_eval_from_mem(std::vector<ranking_t> const& evals, mask_set const* ms, size_t single_mask)noexcept{
                                (void)single_mask;
                                int non_zero{ 0 };
                                for(auto& _ : subs_){
                                        auto weight = generic_weight_policy{}
                                                .calculate(_->hand_mask(), *ms);
                                        if constexpr ( CheckZeroWeight ){
                                                if( weight == 0 )
                                                        continue;
                                        }
                                        _->accept_weight(weight, evals);
                                        ++non_zero;
                                }
                                return non_zero;
                        }
                private:
                        std::vector<ranking_t> evals_;
                        std::vector<SubPtrType>& subs_;
                };
        };

        template<>
        struct supports_single_mask<generic_shed> : std::true_type {};
} // end namespace ps

#endif // LIB_EVAL_GENERIC_SHED_H
