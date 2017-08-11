#ifndef PS_SUPPORT_PROC_H
#define PS_SUPPORT_PROC_H

#include "ps/support/push_pull.h"

#include <list>

namespace ps{
namespace support{


struct single_processor{
        using work_t = std::function<void()>;
        single_processor()
                :work_{static_cast<size_t>(-1)}
                ,wh_(work_.make_work_handle())
        {
        }
        ~single_processor(){
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

} // support
} // ps

#endif // PS_SUPPORT_PROC_H
