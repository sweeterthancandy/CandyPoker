#ifndef PS_EVAL_CLASS_EQUITY_EVALUATOR_DEFAULT_H
#define PS_EVAL_CLASS_EQUITY_EVALUATOR_DEFAULT_H

#include "ps/eval/class_equity_evaluator.h"

namespace ps{

        struct class_equity_evaluator_default : class_equity_evaluator{

                std::shared_ptr<equity_breakdown> evaluate_class(holdem_class_vector const& cv)const override{
                        if( class_cache_ ){
                                auto ptr = class_cache_->try_lookup_perm(cv);
                                if( ptr )
                                        return ptr;
                        }
                        
                        auto t = cv.to_standard_form();
                        auto const& perm = std::get<0>(t);
                        auto const& vec  = std::get<1>(t);

                        auto result = evaluate_class_impl(vec);
                
                        if( class_cache_ ){
                                class_cache_->lock();
                                class_cache_->commit( vec, *result);
                                class_cache_->unlock();
                        }

                        return std::make_shared<equity_breakdown_matrix>(*result, perm);
                }
                void inject_class_cache(std::shared_ptr<holdem_class_eval_cache> ptr)override{
                        class_cache_ = ptr;
                }
        protected:
                virtual std::shared_ptr<equity_breakdown> evaluate_class_impl(holdem_class_vector const& players)const=0;
        private:
                std::shared_ptr<holdem_class_eval_cache>  class_cache_;
        };
} // ps

#endif // PS_EVAL_CLASS_EQUITY_EVALUATOR_DEFAULT_H 

