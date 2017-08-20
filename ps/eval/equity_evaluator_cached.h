#ifndef PS_EVAL_EQUITY_CALCULATOR_CACHED_H
#define PS_EVAL_EQUITY_CALCULATOR_CACHED_H

#include "ps/base/algorithm.h"
#include "ps/eval/equity_evaluator.h"

namespace ps{

struct equity_evaluator_cached : public equity_evaluator{
        equity_evaluator_cached()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {}
        std::shared_ptr<equity_breakdown> evaluate(std::vector<holdem_id> const& players)const override{
                auto p =  permutate_for_the_better(players) ;
                auto& perm = std::get<0>(p);
                auto const& perm_players = std::get<1>(p);
                
                auto iter = cache_.find(perm_players);
                if( iter != cache_.end() ){
                        return std::make_shared<equity_breakdown_permutation_view>(
                                iter->second,
                                std::move(perm)
                        );
                }

                auto principal_result = impl_->evaluate( perm_players );
                cache_.emplace(perm_players, principal_result);
                return evaluate(players);
        }
private:
        equity_evaluator* impl_;
        mutable std::map<std::vector<holdem_id> , std::shared_ptr<equity_breakdown> > cache_;
};

} // ps
#endif // PS_EVAL_EQUITY_CALCULATOR_CACHED_H
