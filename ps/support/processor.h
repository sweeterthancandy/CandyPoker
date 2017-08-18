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

#include "ps/support/config.h"

namespace ps{
namespace support{
// The idea here is that there are two types of multi-threaded
// computation that I want to carry out, those disjoint computations,
// but also joint computations.
//
//
#if 0
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
#endif
struct processor{

        using work_t = std::function<void()>;

        enum Ctrl{
                Ctrl_Success,                    // When there is something to schedule
                Ctrl_NothingLeftButNotFinished,  // When everything is scheduled, but not finished
                Ctrl_NotReady,                   // When we're waiting for a task to finish first
                Ctrl_Finished                    // When everything has been scheduled and finished     
        };

        struct schedulable : std::enable_shared_from_this<schedulable>{
                virtual std::tuple<Ctrl, boost::optional<work_t> > select()=0;
        };

        struct single_task : schedulable{
                enum State{
                        State_ReadyToRun,
                        State_Selected,
                        State_Finished
                };
                explicit single_task(work_t w)
                        :w_{std::move(w)}
                {
                }
                std::tuple<Ctrl, boost::optional<work_t> > select()override{
                        switch(state_){
                        case State_ReadyToRun:
                                state_ = State_Selected;
                                return std::make_tuple(
                                        Ctrl_Success,
                                        work_t{[_w=std::move(w_),handle=shared_from_this(),this](){
                                                _w();
                                                state_ = State_Finished;
                                        }}
                                );
                        case State_Selected:
                                return std::make_tuple(
                                        Ctrl_NothingLeftButNotFinished,
                                        boost::none
                                );
                        case State_Finished:
                                return std::make_tuple(
                                        Ctrl_Finished,
                                        boost::none
                                );
                        }
                        PS_UNREACHABLE();
                }
        private:
                std::atomic_int state_{State_ReadyToRun};
                work_t w_;
        };


        struct unsequenced_group : schedulable{
                // Returns
                //      
                //      Ctrl_Success                    => one child has something to select()
                //      Ctrl_NotReady                   => no child had something to select(), 
                //                                         but one or more have something 
                //                                         soon
                //      Ctrl_NothingLeftButNotFinished  => have nothing left to select(), but
                //                                         some children are still running
                //      Ctrl_Finished                   => every child has finished
                //
                void push(std::shared_ptr<schedulable> ptr){
                        vec_.push_back(std::move(ptr));
                }

                std::tuple<Ctrl, boost::optional<work_t> > select()override{
                        // They get selected in order ie
                        //     {p0_Start, p1_start, p2_start,...}
                        
                        Ctrl default_reason = Ctrl_Finished;
                        for( auto& ptr : vec_){
                                // There might be something to schedule,
                                // but it might not be ready
                                auto ret = ptr->select();
                                switch(std::get<0>(ret)){
                                case Ctrl_Success:
                                        ++selected_;
                                        return std::make_tuple(
                                                Ctrl_Success,
                                                work_t{[_w=std::get<1>(ret).get(),handle=shared_from_this(),this](){
                                                        _w();
                                                        ++done_;
                                                }}
                                        );
                                case Ctrl_NotReady:
                                        default_reason = Ctrl_NotReady;
                                        continue;
                                case Ctrl_NothingLeftButNotFinished:
                                        // If there's already a reason, then the current
                                        // reason is better
                                        if( default_reason == Ctrl_Finished )
                                                default_reason = Ctrl_NothingLeftButNotFinished;
                                        continue;
                                case Ctrl_Finished:
                                        continue;
                                }
                                PS_UNREACHABLE();
                        }
                        return std::make_tuple(
                                default_reason,
                                boost::none
                        );
                }
        private:
                std::atomic_int selected_{0};
                std::atomic_int done_{0};
                std::vector<std::shared_ptr<schedulable> > vec_;
        }; 
        
        struct sequenced_group : schedulable{
                void push(std::shared_ptr<schedulable> ptr){
                        vec_.push_back(std::move(ptr));
                }
                std::tuple<Ctrl, boost::optional<work_t> > select()override{
                        // They get selected in order ie
                        //     {p0_Start, p0_end, p1_start, p1_end,...}
                        
                        for(;iter_ < vec_.size();){

                                auto ret = vec_[iter_]->select();
                                switch(std::get<0>(ret)){
                                case Ctrl_Success:
                                        ++selected_;
                                        return std::make_tuple(
                                                Ctrl_Success,
                                                work_t{[_w=std::get<1>(ret).get(),handle=shared_from_this(),this](){
                                                        _w();
                                                        ++done_;
                                                }}
                                        );
                                case Ctrl_NotReady:
                                        return std::move(ret);
                                case Ctrl_NothingLeftButNotFinished:
                                        return std::make_tuple(
                                                ( iter_ + 1 == vec_.size() ?
                                                        Ctrl_NothingLeftButNotFinished :
                                                        Ctrl_NotReady ),
                                                boost::none
                                        );
                                case Ctrl_Finished:
                                        ++iter_;
                                        continue;
                                }
                                PS_UNREACHABLE();
                        }
                        // We only get here by a sequence of zero or
                        // more Ctrl_Finished
                        return std::make_tuple(
                                Ctrl_Finished,
                                boost::none
                        );
                }
        private:
                size_t iter_{0};
                std::atomic_int selected_{0};
                std::atomic_int done_{0};
                std::vector<std::shared_ptr<schedulable> > vec_;
        }; 

        // This is just for creationable purposes,
        // because we only every create process_groups in
        // the form
        //
        //      auto pg0 = std::make_shared<process_group>();
        //      pg0->add(f);
        //      pg0->add(g);
        //      pg0->sequence_point();
        //      pg0->add(h);
        //      auto pg1 = std::make_shared<process_group>();
        //      pg1->add(pg1);
        //      pg1->add(i);
        //      pg1->sequence_point();
        //      pg1->add(j);
        //
        //      proc.accept(pg1);
        //      
        struct process_group{
                process_group()
                        :group_{std::make_shared<sequenced_group>()}
                {}

                /////////////////////////////////////////////////////////
                //
                //           Creational interface
                //
                /////////////////////////////////////////////////////////
                void sequence_point(){
                        if( working_stack_.empty())
                                return;
                        auto g = std::make_shared<unsequenced_group>();
                        for( auto& ptr : working_stack_ ){
                                g->push( std::move(ptr) );
                        }
                        group_->push(std::move(g));
                        working_stack_.clear();
                }
                auto compile(){
                        this->sequence_point();
                        return std::move(group_);
                }
                void push(work_t w){
                        working_stack_.push_back(
                                std::make_shared<single_task>(std::move(w))
                        );
                }
                void push(std::unique_ptr<process_group> pg){
                        working_stack_.push_back(
                                pg->compile()
                        );
                }
        private:
                std::shared_ptr<sequenced_group> group_;
                std::vector< std::shared_ptr<schedulable> > working_stack_;
        };

        processor()
                :head_{std::make_shared<unsequenced_group>()}
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
                assert( group.get() != nullptr && "precondition failed");
                //std::cout << "accept()\n";
                std::unique_lock<std::mutex> lock(mtx_);
                head_->push(group->compile());
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
        void __main__(){
                //std::cout << "__main__()\n";
                for(;;){
                        decltype(this->head_->select()) ret;
                        {
                                //std::cout << "locking()\n";
                                std::unique_lock<std::mutex> lock(mtx_);
                                ret = head_->select();
                                switch(std::get<0>(ret)){
                                case Ctrl_Success:
                                        break;
                                case Ctrl_NotReady:
                                case Ctrl_NothingLeftButNotFinished:
                                case Ctrl_Finished:
                                        if( ! running_ )
                                                return;
                                        no_work_barrier_.wait(lock);
                                        continue;
                                }
                        }
                        std::get<1>(ret).get()();
                        std::unique_lock<std::mutex> lock(mtx_);
                        no_work_barrier_.notify_all();
                }
        }
        std::mutex mtx_;
        std::condition_variable no_work_barrier_;
        std::thread schedular_thread_;
        std::vector<std::thread> threads_;
        std::atomic_bool running_{true};
        std::shared_ptr<unsequenced_group> head_;
};
} // support
} // ps

#if 0
auto make_proto(std::string const& tok){
        auto a = std::make_unique<processor::process_group>();
        a->push( [tok](){
                std::cout << tok << "::1\n";
        });
        a->push( [tok](){
                std::cout << tok << "::2\n";
        });
        a->sequence_point();
        a->sequence_point();
        a->push( [tok](){
                std::cout << tok << "::3\n";
        });
        return std::move(a);
}

int main(){
        processor proc;
        for(int i=0;i!=100;++i)
                proc.spawn();
        #if 1
        proc.accept(make_proto("a"));
        proc.accept(make_proto("b"));
        proc.accept(make_proto("c"));
        #endif
        proc.join();
}
#endif

#endif // PS_SUPPORT_PROCESSOR_H
