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
                        void begin_eval(mask_set const& ms)noexcept{
                                ms_   = &ms;
                                out_  = 0;
                        }
                        void put(size_t index, ranking_t rank)noexcept{
                                evals_[index] = rank;
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
                        size_t batch_size_;
                        std::vector<ranking_t> evals_;
                        std::vector<SubPtrType>& subs_;
                        mask_set const* ms_;
                        size_t out_{0};
                };
        };

} // end namespace ps

#endif // LIB_EVAL_GENERIC_SHED_H
