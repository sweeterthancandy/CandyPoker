#ifndef PS_SUPPORT_PUSH_PULL_H
#define PS_SUPPORT_PUSH_PULL_H

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace ps{

/*
        This is pretty involved, need to abstract the producer consumer 
        model when I have a better idea of the abstractions
 */

namespace support{

template<class T>
struct push_pull{
        using work_type = T;

        struct handle{
                handle():This_{0}{}
                handle(push_pull* This):This_{This}{
                        This_->begin_worker();
                }
                ~handle(){
                        if( !! This_ ){
                                unlock();
                        }
                }
                handle(handle&& that){
                        this->This_ = that.This_;
                        that.This_ = 0;
                }
                handle& operator=(handle&& that){
                        this->This_ = that.This_;
                        that.This_ = 0;
                        return *this;
                }
                
                // don't want these
                handle(handle const& that)=delete;
                handle& operator=(handle const& that)=delete;

                void unlock(){
                        assert( !!This_ && "preconditon failed");
                        This_->end_worker();
                        This_ = 0;
                }
        private:
                push_pull* This_;
        };

        push_pull(size_t max_work = 0):max_work_{max_work},workers_working_{0}{}

        handle make_work_handle(){ return handle{this}; }
        /*
         * Workers interface
         */
        void begin_worker(){
                std::unique_lock<std::mutex> lock(this->mtx_);
                ++workers_working_;
                this->pull_cond_.notify_all();
        }
        void end_worker(){
                std::unique_lock<std::mutex> lock(this->mtx_);
                --workers_working_;
                this->pull_cond_.notify_all();
        }
        void push(work_type const& work){
                std::unique_lock<std::mutex> lock(this->mtx_);
                if( this->work_.size() > max_work_ ){
                        std::cerr << "waiting\n";
                        this->push_cond_.wait(lock);
                }
                this->work_.push_back(work);
                this->pull_cond_.notify_one();
        }
        
        
        /*
         * Pullers interface
         */
        boost::optional<work_type> pull(){
                std::unique_lock<std::mutex> lock(this->mtx_);
                // need this because when waiting for a pull, we 
                // get notificaiton in the form of pull_cond_
                // that workers_working_ changed (maybe
                // this should only be when it's zero)
                for(;;){
                        if( this->work_.empty() ){
                                if( workers_working_ == 0 ){
                                        return boost::none;
                                }

                                this->pull_cond_.wait( lock );
                                continue;
                        }


                        break; // <-------------
                }
                //PRINT_SEQ(( workers_working_)( work_.size()) );
                auto work = std::move(this->work_.back());
                this->work_.pop_back();
                this->push_cond_.notify_all();
                return std::move(work);
        }
private:
        size_t max_work_;

        std::atomic_int         workers_working_;

        std::mutex              mtx_;
        std::vector<work_type>  work_;
        std::condition_variable pull_cond_;
        std::condition_variable push_cond_;
};

#if 0
int main(int argc, char** argv){
        ps::support::push_pull<int> pp;
        std::vector<std::thread> tg;
        size_t n=50;
        auto producer = [&,wh=pp.make_work_handle()]()mutable{
                for(int i=0;i!=n;++i)
                        pp.push(i);
                wh.unlock();
        };
        std::mutex mtx;
        std::vector<int> result;
        auto consumer = [&](){
                for(;;){
                        auto work =  pp.pull() ;
                        if( ! work )
                                break;
                        std::unique_lock<std::mutex> lock(mtx);
                        result.push_back(work.get());
                }
        };

        tg.emplace_back([&](){ producer(); });
        for(size_t i=0;i!=20;++i){
                tg.emplace_back([&](){ consumer(); });
        }

                
        for( auto& t : tg )
                t.join();
        
        boost::sort(result);
        PRINT( ps::detail::to_string(result) );
        PRINT( result.size());
}
#endif
} // support
} // ps

#endif // PS_SUPPORT_PUSH_PULL_H
