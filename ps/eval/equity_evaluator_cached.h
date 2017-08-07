#ifndef PS_EVAL_EQUITY_CALCULATOR_CACHED_H
#define PS_EVAL_EQUITY_CALCULATOR_CACHED_H

#include "ps/base/algorithm.h"
#include "ps/eval/equity_evaluator.h"

#include <future>

namespace ps{

struct equity_evaluator_cached : public equity_evaluator{
        equity_evaluator_cached()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {}
        std::shared_ptr<equity_breakdown> evaluate(std::vector<holdem_id> const& players)const override{
                return this->future_evaluate(players).get();
        }

        std::shared_future<std::shared_ptr<equity_breakdown> > 
        future_evaluate(std::vector<holdem_id> const& players)const{
                std::cout << "equity_evaluator_cached::future_evaluate\n";
                auto p =  permutate_for_the_better(players) ;
                auto& perm = std::get<0>(p);
                auto const& perm_players = std::get<1>(p);
                
                auto iter = cache_.find(perm_players);
                if( iter != cache_.end() ){
                        return std::async( [iter,perm]()mutable
                                           ->std::shared_future<std::shared_ptr<equity_breakdown> > 
                        {
                                std::shared_ptr<equity_breakdown> ptr(
                                        new equity_breakdown_permutation_view(
                                                iter->second.get(),
                                                perm
                                ));
                                return ptr;
                        });
                }

                cache_.emplace(perm_players, std::async( 
                        [perm_players,&eval](){
                        return impl_->evaluate(perm_players);
                });
                return future_evaluate(players);
        }
private:
        equity_evaluator* impl_;
        mutable std::map<
                holdem_hand_vector,
                std::shared_future<std::shared_ptr<equity_breakdown> >
        > cache_;
};

} // ps
#endif // PS_EVAL_EQUITY_CALCULATOR_CACHED_H
