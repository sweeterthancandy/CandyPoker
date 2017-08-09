#ifndef PS_EVAL_EQUITY_FUTURE_H
#define PS_EVAL_EQUITY_FUTURE_H

#include "ps/base/algorithm.h"
#include "ps/eval/equity_evaluator.h"

namespace ps{


struct processor{
        using work_t = std::function<void()>;
        processor()
                :wh_(work_.make_work_handle())
        {
                std::thread([this]()mutable{
                        __main__();
                }).detach();
        }
        ~processor(){
                wh_.unlock();
        }
        void add(work_t w){
                work_.push(std::move(w));
        }
private:
        void __main__(){
                for(;;){
                        auto work = work_.pull() ;
                        if( ! work )
                                break;
                        work();
                }
        }
        support::push_pull<work_t>         work_;
        support::push_pull<work_t>::handle wh_;
};


struct equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        equity_future()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {}
        result_t evaluate(processor& proc, hand_vector const& players)const override{
                auto iter = cache_.find(players);
                if( iter != cache_.end() ){
                        return iter->second;
                }
                std::packaged_task<std::shared_ptr<equity_breakdown>()> task(
                        [players,this](){
                        return impl_->evaluate(players);
                });
                m_.emplace(players, std::move(task.get_future()));
                proc.add(std::move(task));
                return evaluate(proc, players);
        }
private:
        equity_evaluator* impl_;
        std::map< holdem_hand_vector, result_t > m_;
};

} // ps
#endif // PS_EVAL_EQUITY_FUTURE_H
