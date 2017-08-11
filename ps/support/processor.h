#ifndef PS_SUPPORT_PROCESSOR_H
#define PS_SUPPORT_PROCESSOR_H


#include <boost/optional.hpp>
#include <list>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace ps{
namespace support{
// The idea here is that there are two types of multi-threaded
// computation that I want to carry out, those disjoint computations,
// but also joint computations.
//
//
struct processor{

        using work_t = std::function<void()>;

        struct process_group{

                struct sequenced_group{
                        sequenced_group():done_{0}{}
                        void push(work_t w){
                                vec_.push_back([_w=std::move(w),this](){
                                        _w();
                                        ++done_;
                                });
                        }
                        work_t pop(){
                                assert( iter_ != vec_.size() && "preconditon failed");
                                return std::move(vec_[iter_++]);
                        }
                        // left to schedule
                        auto tasks_left()const{
                                return vec_.size() - iter_;
                        }
                        // left to actually finish
                        bool all_tasks_finished()const{
                                return done_ == vec_.size();
                        }
                private:
                        std::atomic_int done_;
                        std::vector<work_t> vec_;
                        size_t iter_ = 0;
                };

                process_group(){
                        sequence_point();
                }

                /////////////////////////////////////////////////////////
                //
                //           Creational interface
                //
                /////////////////////////////////////////////////////////
                void sequence_point(){
                        seq_.emplace_back(std::make_unique<sequenced_group>());
                }
                void push(work_t w){
                        seq_.back()->push(std::move(w));
                }


                /////////////////////////////////////////////////////////
                //
                //           Interface for schedular 
                //
                /////////////////////////////////////////////////////////
                auto groups_left()const{
                        return seq_.size() - iter_;
                }
                auto& head(){
                        return *seq_[iter_];
                }
                void pop(){
                        assert( iter_ < seq_.size() && "precondition failed");
                        ++iter_;
                }
        private:
                std::vector< std::unique_ptr<sequenced_group> > seq_;
                size_t iter_ = 0;
        };

        processor()
        {
        }
        ~processor(){
                join(); // maybe
        }
        void join(){
                //std::cout << "join()\n";
                running_ = false;
                no_work_barrier_.notify_all();
                if( threads_.size() ){
                        for( auto& t : threads_ )
                                t.join();
                        threads_.clear();
                }
        }
        void accept( std::unique_ptr<process_group> group ){
                //std::cout << "accept()\n";
                std::unique_lock<std::mutex> lock(mtx_);
                groups_.emplace_back(std::move(group));
                no_work_barrier_.notify_all();
        }
        void spawn(){
                //std::cout << "spawn()\n";
                threads_.emplace_back(
                        [this]()mutable{
                        __main__();
                });
        }
private:
        boost::optional<work_t> schedule_(){
                //std::cout << "schedule_()\n";
                using GI = decltype(groups_.begin());

                // I could go return this->schedule_() rather than goto,
                // but then we can get stack overflow
                retry_:;

                // go thought the groups, FIFO, to find a group with a task
                // to schedual
                for( GI iter(groups_.begin()), end(groups_.end());iter!=end;++iter){
                        for(;;){
                                process_group& pg(**iter);
                                if( pg.groups_left() == 0 ){
                                        // Here the group is done,
                                        groups_.erase(iter);
                                        no_work_barrier_.notify_all();
                                        goto retry_;
                                }

                                // we have an active group
                                if( pg.head().tasks_left() != 0 ){
                                        auto ret = pg.head().pop();
                                        return std::move(ret);
                                }

                                if( pg.head().all_tasks_finished() ){
                                        pg.pop();
                                        continue;
                                }
                                break;
                        }
                }
                return boost::none;
        }
        void __main__(){
                //std::cout << "__main__()\n";
                for(;;){
                        decltype(this->schedule_()) work;
                        {
                                //std::cout << "locking()\n";
                                std::unique_lock<std::mutex> lock(mtx_);
                                work = schedule_();
                                //std::cout << "schedule_() => " << work.operator bool() << "\n";
                                if( ! work ){
                                        if( ! running_ && groups_.size() == 0 )
                                                return;
                                        //std::cout << "waiting\n";
                                        no_work_barrier_.wait(lock);
                                        continue;
                                }
                        }
                        work.get()();
                        no_work_barrier_.notify_all();
                }
        }
        std::mutex mtx_;
        std::condition_variable no_work_barrier_;
        std::thread schedular_thread_;
        std::vector<std::thread> threads_;
        std::list<std::unique_ptr<process_group>> groups_;
        std::atomic_bool running_{true};
};
} // support
} // ps

#endif // PS_SUPPORT_PROCESSOR_H
