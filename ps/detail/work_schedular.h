#ifndef PS_DETAIL_WORK_SCHEDULER_H
#define PS_DETAIL_WORK_SCHEDULER_H

#include <functional>
#include <thread>
#include <mutex>
#include <vector>

namespace ps{
namespace detail{
        // toy schedular
        struct work_scheduler{
                void decl( std::function<void()> work){
                        work_.emplace_back(std::move(work));
                }
                auto run(){
                        std::mutex mtx;
                        std::vector<std::thread> workers;

                        auto proto = [&]()mutable{
                                for(;;){
                                        mtx.lock();
                                        if( work_.empty()){
                                                mtx.unlock();
                                                break;
                                        }
                                        auto w = work_.back();
                                        work_.pop_back();
                                        mtx.unlock();
                                        w();
                                }
                        };
                        for(size_t i=0;i!= std::thread::hardware_concurrency();++i){
                                workers.emplace_back(proto);
                        }
                        for( auto& t : workers){
                                t.join();
                        }
                }
        private:
                std::vector<std::function<void()> > work_;
        };

} /* ps */
} /* detail */

#endif // PS_DETAIL_WORK_SCHEDULER_H
