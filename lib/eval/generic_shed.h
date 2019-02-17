#ifndef LIB_EVAL_GENERIC_SHED_H
#define LIB_EVAL_GENERIC_SHED_H


namespace ps{

        /*
                This was originally meant to be a small interface, and then
                from the small interface be able to change the order of evaluations,
                thinking I could do something in terns or reordering for CPU-cache
                improvment. This didn't work. but this is still helpfull
         */

        struct generic_weight_policy{
                // this is 5% faster (in my limited testing)
                enum{ CheckUnion = true };
                size_t calculate(size_t hv_mask, mask_set const* ms, size_t single_mask)const noexcept{
                        if( !! ms ){
                                if( CheckUnion ){
                                        if( ( ms->get_union() & hv_mask) == 0 ){
                                                return ms->size();
                                        }
                                }
                                return ms->count_disjoint(hv_mask);
                        } else {
                                return (( hv_mask & single_mask )==0?1:0);
                        }
                }
        };

        struct generic_shed{
                template<class SubPtrType>
                struct bind{
                        // this is 5% faster (in my limited testing)
                        enum{ CheckZeroWeight = true };
                        explicit bind(size_t batch_size, std::vector<SubPtrType>& subs)
                                :subs_{subs}
                        {
                                evals_.resize(batch_size);
                        }
                        void put(size_t index, ranking_t rank)noexcept{
                                evals_[index] = rank;
                        }
                        void end_eval(mask_set const* ms, size_t single_mask)noexcept{
                                for(auto& _ : subs_){
                                        size_t weight = generic_weight_policy{}
                                                .calculate(_->hand_mask(), ms, single_mask);
                                        #if 0
                                        size_t weight = [&]()->size_t{
                                                auto hv_mask = _->hand_mask();
                                                if( !! ms ){
                                                        if( CheckUnion ){
                                                                if( ( ms->get_union() & hv_mask) == 0 ){
                                                                        return ms->size();
                                                                }
                                                        }
                                                        return ms->count_disjoint(hv_mask);
                                                } else {
                                                        return (( hv_mask & single_mask )==0?1:0);
                                                }
                                        }();
                                        #endif
                                        if( CheckZeroWeight ){
                                                if( weight == 0 )
                                                        continue;
                                        }
                                        _->accept_weight(weight, evals_);
                                }
                        }
                        void regroup()noexcept{
                                // nop
                        }
                private:
                        std::vector<ranking_t> evals_;
                        std::vector<SubPtrType>& subs_;
                };
        };

} // end namespace ps

#endif // LIB_EVAL_GENERIC_SHED_H
