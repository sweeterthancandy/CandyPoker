#ifndef PS_EVAL_EQUITY_FUTURE_H
#define PS_EVAL_EQUITY_FUTURE_H

#include <functional>
#include <future>

#include "ps/support/push_pull.h"
#include "ps/base/algorithm.h"
#include "ps/eval/equity_evaluator.h"

namespace ps{


struct processor{
        using work_t = std::function<void()>;
        processor()
                :work_{static_cast<size_t>(-1)}
                ,wh_(work_.make_work_handle())
        {
        }
        ~processor(){
        }
        void add(work_t w){
                work_.push(std::move(w));
        }
        void join(){
                wh_.unlock();
                for( auto& t : threads_ )
                        t.join();
                threads_.clear();
        }
        void run(){
                for(;;){
                        PRINT(work_.size());
                        auto work = work_.pull_no_wait();
                        if( ! work )
                                break;
                        work.get()();
                }
        }
        void spawn(){
                threads_.emplace_back(
                        [this]()mutable{
                        __main__();
                });
        }
private:
        void __main__(){
                for(;;){
                        PRINT(work_.size());
                        auto work = work_.pull() ;
                        if( ! work )
                                break;
                        work.get()();
                }
        }
        support::push_pull<work_t>         work_;
        support::push_pull<work_t>::handle wh_;
        std::vector<std::thread> threads_;
};


struct equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        equity_future()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {}
        result_t schedual(processor& proc, holdem_hand_vector const& players){
                auto iter = m_.find(players);
                if( iter != m_.end() ){
                        return iter->second;
                }
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [players,this](){
                        return impl_->evaluate(players);
                });
                m_.emplace(players, std::move(task->get_future()));

                processor::work_t w = [task]()mutable{
                        (*task)();
                };
                proc.add(std::move(w));
                return schedual(proc, players);
        }
private:
        equity_evaluator* impl_;
        std::map< holdem_hand_vector, result_t > m_;
};

} // ps
#endif // PS_EVAL_EQUITY_FUTURE_H
