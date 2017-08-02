#include "ps/calculator_detail.h"
#include "ps/calculator.h"
#include "ps/heads_up.h"
#include "ps/frontend.h"

#include "ps/support/push_pull.h"

#include <boost/timer/timer.hpp>

#include <mutex>
#include <thread>
#include <condition_variable>

namespace ps{


template<class T, size_t N>
struct combination_producer{
        void operator()(size_t max_value, support::push_pull<std::vector<std::vector<T>> >* pp){
                size_t batch_size{50};
                std::vector<std::vector<T>> buffer;

                auto push{[&](){
                        std::cout << detail::to_string( buffer.front() ) << "\n";
                        pp->push(std::move(buffer) );
                        buffer.clear();
                }};

                detail::visit_combinations<N>(
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


                        buffer.emplace_back(
                                std::vector<T>{
                                        static_cast<holdem_id>(args)...});
                        if( buffer.size() == batch_size ){
                                push();
                        }
                }, detail::true_, max_value);

                if( buffer.size() != 0 ){
                        push();
                }
        }
};


struct create_cache_driver
{
        using work_type = std::vector< std::vector<holdem_id> >;

        void run(){
                //auto num_threads =  std::thread::hardware_concurrency()*2;
                auto num_threads = 10;
                std::vector<std::thread> tg;


                #if 0
                tg.emplace_back(
                        [this,wh=holdem_hand_workspace_.make_work_handle()]()mutable
                        {
                                combination_producer<holdem_id, 2>{}(holdem_hand_decl::max_id-1, &holdem_hand_workspace_);
                                combination_producer<holdem_id, 3>{}(holdem_hand_decl::max_id-1, &holdem_hand_workspace_);
                                wh.unlock();
                        } 
                );
                #endif
                tg.emplace_back(
                        [this,wh=holdem_class_workspace_.make_work_handle()]()mutable
                        {
                                combination_producer<holdem_class_id, 2>{}(holdem_class_decl::max_id-1, &holdem_class_workspace_);
                                #if 0
                                combination_producer<holdem_class_id, 3>{}(holdem_class_decl::max_id-1, &holdem_class_workspace_);
                                combination_producer<holdem_class_id, 4>{}(holdem_class_decl::max_id-1, &holdem_class_workspace_);
                                #endif
                                wh.unlock();
                        } 
                );

                for(size_t i=0; i!= num_threads; ++i){
                        tg.emplace_back(
                                [this,hw=aggregate_workspace_.make_work_handle()]()mutable
                                {
                                        auto calc =  std::make_shared<calculater>() ;
                                        for(;;){
                                                auto work = holdem_hand_workspace_.pull();
                                                if( ! work )
                                                        break;

                                                for( auto v : work.get() ){
                                                        //calc->calculate_hand_equity( v );
                                                        ++done_;
                                                }
                                        }
                                        aggregate_workspace_.push(std::move(calc));
                                        hw.unlock();
                                }
                        );
                        tg.emplace_back(
                                [this,hw=aggregate_workspace_.make_work_handle()]()mutable
                                {
                                        auto calc =  std::make_shared<calculater>() ;
                                        for(;;){
                                                auto work = holdem_class_workspace_.pull();
                                                if( ! work )
                                                        break;

                                                for( auto v : work.get() ){
                                                        calc->calculate_class_equity( v );
                                                        ++done_;
                                                }
                                        }
                                        aggregate_workspace_.push(std::move(calc));
                                        hw.unlock();
                                }
                        );
                }
                calculater aggregate;
                tg.emplace_back(
                        [&]()
                        {
                                for(;;){
                                        auto ret =  aggregate_workspace_.pull() ;
                                        if( ! ret ){
                                                break;
                                        }
                                        aggregate.append(**ret);
                                }
                                std::cerr << "aggregator finished\n";
                        }
                );

                for( auto& t : tg )
                        t.join();

                aggregate.save("3newcache.bin_");
        }
private:
        #if 0
        void producer_(){
                size_t batch_size{50};
                work_type buffer;

                auto push{[&](){
                        std::cout << detail::to_string( buffer.front() ) << "\n";
                        workspace_.push(std::move(buffer) );
                        buffer.clear();
                }};

                detail::visit_combinations<2>(
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
        }
        #endif
        #if 0
        void consumer_(){
                calculater calc;
                for(;;){
                        auto work = holdem_hand_workspace_.pull();
                        if( ! work )
                                break;

                        for( auto v : work.get() ){
                                //calc.calculate_class_equity( v );
                                ++done_;
                        }
                }
                std::unique_lock<std::mutex> lock(this->mtx_);
                this->result_.push_back(std::move(calc));
        }
        #endif
private:
        support::push_pull< std::vector<std::vector<holdem_id> > >       holdem_hand_workspace_;
        support::push_pull< std::vector<std::vector<holdem_class_id> > > holdem_class_workspace_;
        support::push_pull< std::shared_ptr<calculater> >                aggregate_workspace_;

        std::mutex mtx_;

        std::vector<calculater> result_;
        bool finished_ = false;

        size_t done_ = 0;
};

void create_cache_main(){
        create_cache_driver driver;
        driver.run();
}

} // ps

int main(int argc, char** argv){
        ps::create_cache_main();
}
