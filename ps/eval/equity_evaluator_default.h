#ifndef PS_EVAL_EQUITY_EVALUATOR_DEFAULT_H
#define PS_EVAL_EQUITY_EVALUATOR_DEFAULT_H

#include "ps/eval/equity_evaluator.h"

namespace ps{

        struct equity_evaluator_default : equity_evaluator{

                std::shared_ptr<equity_breakdown> evaluate(std::vector<holdem_id> const& hv_)const override{
                        holdem_hand_vector hv{hv_};

                        //return evaluate_impl(hv);
                        if( cache_ ){
                                auto ptr = cache_->try_lookup_perm(hv);
                                if( ptr )
                                        return ptr;
                        }
                        
                        auto t = hv.to_standard_form();
                        auto const& perm = std::get<0>(t);
                        auto const& vec  = std::get<1>(t);

                        auto result = evaluate_impl(vec);
                
                        if( cache_ ){
                                cache_->lock();
                                cache_->commit( vec, *result);
                                cache_->unlock();
                        }

                        return std::make_shared<equity_breakdown_matrix>(*result, perm);
                }
                void inject_cache(std::shared_ptr<holdem_eval_cache> ptr)override{
                        cache_ = ptr;
                }
        protected:
                virtual std::shared_ptr<equity_breakdown> evaluate_impl(holdem_hand_vector const& players)const=0;
        private:
                std::shared_ptr<holdem_eval_cache>  cache_;
        };
} // ps

#endif // PS_EVAL_EQUITY_EVALUATOR_DEFAULT_H 

