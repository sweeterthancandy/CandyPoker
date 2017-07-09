#include "ps/calculator_detail.h"
#include "ps/calculator.h"
#include "ps/heads_up.h"
#include "ps/frontend.h"

#include <boost/timer/timer.hpp>

#include <mutex>
#include <thread>
#include <condition_variable>

namespace ps{

/*
        This is pretty involved, need to abstract the producer consumer 
        model when I have a better idea of the abstractions
 */

template<class T>
struct push_pull{
        using work_type = T;

        push_pull():workers_working_{0}{}

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
                if( this->work_.size() != 0 ){
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
                if( this->work_.empty() ){
                        if( workers_working_ != 0 ){
                                this->pull_cond_.wait( lock );
                        }
                }
                if( this->work_.empty() ){
                        return boost::none;
                }
                auto work{std::move(this->work_.back())};
                this->work_.pop_back();
                this->push_cond_.notify_all();
                return std::move(work);
        }
private:

        std::atomic_int         workers_working_;

        std::mutex              mtx_;
        std::vector<work_type>  work_;
        std::condition_variable pull_cond_;
        std::condition_variable push_cond_;
};


struct create_cache_driver
{
        using work_type = std::vector< std::vector<holdem_id> >;

        void run(){
                std::vector<std::thread> tg;

                tg.emplace_back([this](){ producer_(); });

                auto num_threads{ std::thread::hardware_concurrency()*2};
                for(size_t i=0; i!= num_threads; ++i){
                        tg.emplace_back([this](){ consumer_(); });
                }

                for( auto& t : tg )
                        t.join();


                #if 0
                calculater aggregate;
                for( auto const& r : result_ )
                        aggregate.append(r);
                aggregate.save("3newcache.bin_");
                #endif
        }
private:
        void producer_(){
                size_t batch_size{50};
                work_type buffer;
                auto push{[&](){
                        std::cout << detail::to_string( buffer.front() ) << "\n";
                        std::unique_lock<std::mutex> lock(this->mtx_);
                        this->work_.push_back(std::move(buffer) );
                        buffer.clear();
                        this->cond_.notify_one();
                }};
                detail::visit_combinations<4>(
                        [&](auto... args)
                {
                        // ok we can't create 
                        //      169! / (169-n)! / batch_size
                        // packets of work at once, we want
                        // to create a wait untill it's need.
                        //
                        //  This is excaclty the sittation 
                        //  coroutines can be used
                        //    

                        // if there's already work, wait untill it's needed
                        std::unique_lock<std::mutex> lock(this->mtx_);
                        if( this->work_.size() != 0 ){
                                this->producer_cond_.wait(lock);
                        }
                        lock.unlock();

                        buffer.emplace_back(
                                std::vector<holdem_id>{
                                        static_cast<holdem_id>(args)...});
                        if( buffer.size() == batch_size ){
                                push();
                        }
                }, detail::true_, holdem_class_decl::max_id -1 );
                if( buffer.size() != 0 ){
                        push();
                }
                std::unique_lock<std::mutex> lock(this->mtx_);
                finished_ = true;
                this->cond_.notify_all();
        }
        void consumer_(){
                calculater calc;
                for(;;){
                        work_type work;

                        std::unique_lock<std::mutex> lock(this->mtx_);
                        if( this->work_.empty() && (! finished_) )
                                this->cond_.wait( lock );
                        if( this->work_.size() ){
                                work = std::move(this->work_.back());
                                this->work_.pop_back();
                                this->producer_cond_.notify_all();
                        } else if( finished_  ){
                                break;
                        } else{
                                std::cerr << "OMG ERROR\n";
                                break;
                        }
                        lock.unlock();

                        for( auto v : work ){
                                //calc.calculate_class_equity( v );
                                ++done_;
                        }

                }
                std::unique_lock<std::mutex> lock(this->mtx_);
                this->result_.push_back(std::move(calc));
        }
private:

        std::vector<work_type> work_;

        std::mutex mtx_;
        std::condition_variable cond_;
        std::condition_variable producer_cond_;

        std::vector<calculater> result_;
        bool finished_ = false;

        size_t done_ = 0;
};

void create_cache_main(){
        create_cache_driver driver;
        driver.run();
}

} // ps

#if 0
int main(int argc, char** argv){
        ps::push_pull<int> pp;
        std::vector<std::thread> tg;
        size_t n=1000;
        auto producer = [&](){
                for(int i=0;i!=n;++i)
                        pp.push(i);
        };
        std::mutex mtx;
        std::vector<int> result;
        auto consumer = [&](){
                for(;;){
                        auto work{ pp.pull() };
                        if( ! work )
                                break;
                        std::unique_lock<std::mutex> lock(mtx);
                        result.push_back(work.get());
                }
        };

        pp.begin_worker();
        tg.emplace_back([&](){ producer(); });
        for(size_t i=0;i!=20;++i){
                tg.emplace_back([&](){ consumer(); });
        }
        pp.end_worker();

                
        for( auto& t : tg )
                t.join();
        
        boost::sort(result);
}
#endif
int main(int argc, char** argv){
        ps::create_cache_main();
}
