#include <iostream>
#include <string>
#include <type_traits>
#include <chrono>
#include "ps/support/processor.h"
#include "ps/base/holdem_hand_vector.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/class_equity_evaluator_cache.h"
#include "ps/eval/class_range_equity_evaluator.h"
#include "ps/detail/cross_product.h"


#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>

using namespace ps;


namespace asio = boost::asio;

struct holdem_class_eval_cache{

        equity_breakdown* lookup(holdem_class_vector const& vec){
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
        }
        void commit(holdem_class_vector vec, equity_breakdown const& breakdown){
                cache_.emplace(std::move(vec), breakdown);
        }

        void display()const{
                for( auto const& item : cache_ ){
                        PRINT(item.first);
                        std::cout << item.second << "\n";
                }
        }

        // work with std::lock_guard etc
        void lock(){
                mtx_.lock();
        }
        void unlock(){
                mtx_.unlock();
        }
private:
        std::mutex mtx_;
        std::map< holdem_class_vector, equity_breakdown_matrix> cache_;
};

struct aggregator_something : std::enable_shared_from_this<aggregator_something>
{
        aggregator_something(size_t n)
        {
                ptr_ = std::make_shared<equity_breakdown_matrix_aggregator>(n);
                fut_ = promise_.get_future();
        }
        void append( equity_breakdown const& breakdown , std::vector<int> const& mat){
                assert( count_ > 0 && "preconditon failed");
                std::lock_guard<std::mutex> lock(mtx_);
                PRINT_SEQ((start_)(count_));
                ptr_->append_matrix( breakdown, mat);
                if( start_ && --count_ == 0 ){
                        promise_.set_value( ptr_ );
                        for( auto& f : post_hook_ )
                                f();
                }
        }
        auto get_future(){ return fut_; }
        void push_post_hook(std::function<void()> f){
                post_hook_.push_back(std::move(f));
        }
        void start(){
                start_ = true;
                if( start_ && count_ == 0 ){
                        promise_.set_value( ptr_ );
                        for( auto& f : post_hook_ )
                                f();
                }
        }
        void up(){ ++count_; }
private:
        bool start_{false};
        std::mutex mtx_;
        size_t count_{0};
        std::shared_ptr<equity_breakdown_matrix_aggregator> ptr_;
        std::promise<std::shared_ptr<equity_breakdown> > promise_;
        std::shared_future<std::shared_ptr<equity_breakdown> > fut_;
        std::vector<std::function<void()> > post_hook_;
};

struct equity_future_eval{       
        equity_future_eval(){
        }
        
        #if 0
        std::shared_ptr<aggregator_something> make_class_vector_future(asio::io_service& io, holdem_class_vector const& vec){
                auto stdvec = vec.to_standard_form();
                auto const& perm     = std::get<0>(stdvec);
                auto const& perm_vec = std::get<1>(stdvec);
                auto stdhands = perm_vec.to_standard_form_hands(); 
                auto impl = std::make_shared<aggregator_something>(perm_vec.size(), stdhands.size());
                auto stdhands = perm_vec.to_standard_form_hands(); 
                for( auto const& stdhand : stdhands ){

                        auto const& hand_perm = std::get<0>(stdhand);
                        auto const& hand_vec  = std::get<1>(stdhand);

                        PRINT( hand_vec );

                        io.post( [impl, hand_perm, hand_vec]()mutable{
                                 auto const& eval = equity_evaluator_factory::get("principal");
                                 auto ret = eval.evaluate(hand_vec);
                                 impl->append( *ret, hand_perm);
                        });
                }
                return impl;
        }
        #endif

        #if 0
        std::shared_future<
                std::shared_ptr<equity_breakdown>
        >
        foo(asio::io_service& io, holdem_class_range_vector const& vec){
                auto cv_vec = vec.to_class_standard_form();
                auto impl = std::make_shared<aggregator_something>(vec.size(), cv_vec.size());
                
                for( auto const& cv : cv_vec ){
                        if( class_cache_ )
                        {
                                class_cache_->lock();
                                auto ptr = class_cache_->lookup( cv );
                                class_cache_->unlock();
                                impl->
                        }
                }
        }
        #endif

        std::shared_future<
                std::shared_ptr<equity_breakdown>
        >
        foo(asio::io_service& io, holdem_class_vector const& vec){
                

                auto stdvec = vec.to_standard_form();
                auto const& perm     = std::get<0>(stdvec);
                auto const& perm_vec = std::get<1>(stdvec);
                

                #if 0
                if( class_cache_ )
                {
                        class_cache_->lock();
                        auto ptr = class_cache_->lookup( perm_vec );
                        class_cache_->unlock();
                        if( !! ptr){
                                impl->up();
                                impl->append( *ptr, perm );
                                return impl->get_future();
                        }
                }
                #endif
                
                auto impl = std::make_shared<aggregator_something>(vec.size());

                auto stdhands = perm_vec.to_standard_form_hands(); 
                for( auto const& stdhand : stdhands ){

                        auto const& hand_perm = std::get<0>(stdhand);
                        auto const& hand_vec  = std::get<1>(stdhand);

                        PRINT( hand_vec );

                        impl->up();
                        io.post( [impl, hand_perm, hand_vec]()mutable{
                                 auto const& eval = equity_evaluator_factory::get("principal");
                                 auto ret = eval.evaluate(hand_vec);
                                 impl->append( *ret, hand_perm);
                        });
                }

                impl->start();
                
                auto ret_prom = std::make_shared<std::promise<std::shared_ptr<equity_breakdown> > >();
                std::shared_future<std::shared_ptr<equity_breakdown> > fut(ret_prom->get_future());

                
                // set return value
                impl->push_post_hook(
                        [ret_prom, perm,impl]()mutable
                        {
                                ret_prom->set_value(
                                        std::make_shared<equity_breakdown_permutation_view>(
                                                impl->get_future().get(),
                                                perm));

                        }
                );
                #if 0
                // add to cache
                impl->push_post_hook(
                        [impl,this,perm_vec]()mutable
                        {
                                if( class_cache_ ){
                                        std::unique_lock<holdem_class_eval_cache> lock(*class_cache_);
                                        class_cache_->commit(perm_vec, *impl->get_future().get() );
                                }
                        }
                );
                #endif

                return fut;
        }
        void inject_cache(std::shared_ptr<holdem_class_eval_cache> ptr){
                class_cache_ = ptr;
        }
private:
        std::shared_ptr<holdem_class_eval_cache> class_cache_;
};

int main(){
        boost::timer::auto_cpu_timer at;
        holdem_class_vector players;

        #if 0
        // Hand 0:  30.845%   28.23%  02.61%      3213892992  297127032.00   { TT+, AQs+, AQo+ }
        // Hand 1:  43.076%   40.74%  02.33%      4637673516  265584984.00   { QQ+, AKs, AKo }
        // Hand 2:  26.079%   25.68%  00.40%      2923177728   45324924.00   { TT }
        players.push_back(" TT+, AQs+, AQo+ ");
        players.push_back(" QQ+, AKs, AKo ");
        players.push_back("TT");
        #else
        // Hand 0:  81.547%   81.33%  00.22%        50134104     133902.00   { AA }
        // Hand 1:  18.453%   18.24%  00.22%        11241036     133902.00   { QQ }
        players.push_back("AA");
        players.push_back("QQ");
        #endif

        auto cache = std::make_shared<holdem_class_eval_cache>();

        boost::asio::io_service io;
        //for(int i=0;i!=3;++i){
                equity_future_eval b;
                b.inject_cache(cache);
                auto ret = b.foo(io, players);
                io.run();
                std::cout << *ret.get() << "\n";
        //}
        cache->display();
        

}
