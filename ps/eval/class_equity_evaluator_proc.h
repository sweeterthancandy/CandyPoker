#ifndef PS_EVAL_EQUITY_CALCULATOR_PROC_H
#define PS_EVAL_EQUITY_CALCULATOR_PROC_H

#include "ps/support/proc.h"
#include "ps/base/algorithm.h"
#include "ps/eval/equity_future.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/class_equity_evaluator.h"

namespace ps{

struct class_equity_evaluator_proc : class_equity_evaluator{
        class_equity_evaluator_proc()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {
        }
        std::shared_ptr<equity_breakdown> evaluate(holdem_class_vector const& players)const override{
                support::single_processor proc;
                for( size_t i=0; i!= std::thread::hardware_concurrency();++i)
                        proc.spawn();
                
                using result_t = std::shared_future<
                        std::shared_ptr<equity_breakdown>
                >;
                std::vector< std::tuple< std::vector<int>, result_t > > items;

                for( auto hv : players.get_hand_vectors()){
                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);
                        auto fut = ef_.schedual(proc, perm_players);
                        items.emplace_back(perm, fut);
                }
                proc.join();
                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
                for( auto& t : items ){
                        result->append(
                                *std::make_shared<equity_breakdown_permutation_view>(
                                        std::get<1>(t).get(),
                                        std::get<0>(t)));
                }
                return result;
        }
private:
        equity_evaluator const* impl_;
        mutable equity_future ef_;
};


} // ps
#endif // PS_EVAL_EQUITY_CALCULATOR_PROC_H
