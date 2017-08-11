#ifndef PS_EVAL_EQUITY_FUTURE_H
#define PS_EVAL_EQUITY_FUTURE_H

#include <functional>
#include <future>

#include "ps/support/processor.h"
#include "ps/base/algorithm.h"
#include "ps/eval/equity_evaluator.h"

namespace ps{


struct equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        equity_future()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {}
        result_t schedual_group(support::processor::process_group& pg, holdem_hand_vector const& players){
                auto iter = m_.find(players);
                if( iter != m_.end() ){
                        return iter->second;
                }
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [players,this](){
                        return impl_->evaluate(players);
                });
                m_.emplace(players, std::move(task->get_future()));

                // yuk
                auto w = [task]()mutable{
                        (*task)();
                };
                pg.push(std::move(w));
                return schedual_group(pg, players);
        }
        result_t schedual(support::processor& proc, holdem_hand_vector const& players){
                auto pg = std::make_unique<support::processor::process_group>();
                auto ret = this->schedual_group(*pg, players);
                proc.accept(std::move(pg));
                return std::move(ret);
        }
private:
        equity_evaluator* impl_;
        std::map< holdem_hand_vector, result_t > m_;
};

} // ps
#endif // PS_EVAL_EQUITY_FUTURE_H
