#ifndef LIB_EVAL_GENERIC_SHED_H
#define LIB_EVAL_GENERIC_SHED_H


namespace ps{

        struct generic_shed{
                template<class SubPtrType>
                struct bind{
                        explicit bind(size_t batch_size, std::vector<SubPtrType>& subs)
                                :batch_size_{batch_size}
                                ,subs_{subs}
                        {
                                evals_.resize(batch_size_);
                        }
                        void put(size_t index, ranking_t rank)noexcept{
                                evals_[index] = rank;
                        }
                        void end_eval(mask_set const* ms, size_t single_mask)noexcept{
                                for(auto& _ : subs_){
                                        _->accept(ms, single_mask, evals_);
                                }
                        }
                        void regroup()noexcept{
                                // nop
                        }
                private:
                        size_t batch_size_;
                        std::vector<ranking_t> evals_;
                        std::vector<SubPtrType>& subs_;
                };
        };

} // end namespace ps

#endif // LIB_EVAL_GENERIC_SHED_H
