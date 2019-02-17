#ifndef LIB_EVAL_GENERIC_SHED_H
#define LIB_EVAL_GENERIC_SHED_H


namespace ps{

        /*
                This was originally meant to be a small interface, and then
                from the small interface be able to change the order of evaluations,
                thinking I could do something in terns or reordering for CPU-cache
                improvment. This didn't work. but this is still helpfull
         */
        struct generic_shed{
                template<class SubPtrType>
                struct bind{
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
                                        _->accept(ms, single_mask, evals_);
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
