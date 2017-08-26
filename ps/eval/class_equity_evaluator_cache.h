#ifndef PS_EVAL_EQUITY_CALCULATOR_CACHE_H
#define PS_EVAL_EQUITY_CALCULATOR_CACHE_H

#include "ps/base/algorithm.h"
#include "ps/eval/equity_future.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/holdem_class_eval_cache.h"

namespace ps{


struct class_equity_evaluator_cache : class_equity_evaluator{
        class_equity_evaluator_cache()
        {
        }
        std::shared_ptr<equity_breakdown> evaluate_class(holdem_class_vector const& players)const override{
                if( ! class_cache_ ){
                        return get_impl_()->evaluate_class(players);
                }
                auto t = players.to_standard_form();
                auto& perm         = std::get<0>(t);
                auto& players_perm = std::get<1>(t);

                class_cache_->lock();
                auto ptr = class_cache_->lookup( players_perm );
                class_cache_->unlock();
                if( !! ptr){
                        return std::make_shared<equity_breakdown_matrix>(
                                *ptr,
                                std::move(perm)
                        );
                }
                auto ret = get_impl_()->evaluate_class(players_perm);
                class_cache_->lock();
                class_cache_->commit(players_perm, *ret);
                class_cache_->unlock();
                return std::make_shared<equity_breakdown_matrix>(
                        *ptr,
                        perm
                );
        }
        void inject_class_cache(std::shared_ptr<holdem_class_eval_cache> ptr)override{
                class_cache_ = ptr;
        }
private:
        class_equity_evaluator const* get_impl_()const{
                if( ! impl_ )
                        impl_ = &class_equity_evaluator_factory::get("principal");
                return impl_;
        }
        mutable class_equity_evaluator const* impl_{ nullptr };
        std::shared_ptr<holdem_class_eval_cache> class_cache_;
};


} // ps
#endif // PS_EVAL_EQUITY_CALCULATOR_CACHE_H
