#ifndef PS_EVAL_EQUITY_FUTURE_EVAL_H
#define PS_EVAL_EQUITY_FUTURE_EVAL_H

#include <mutex>
#include <future>
#include <boost/asio.hpp>

#include <boost/variant.hpp>

#include "ps/base/holdem_class_vector.h"
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/holdem_class_eval_cache.h"
#include "ps/eval/equity_evaluator.h"

namespace ps{

struct aggregator_something : std::enable_shared_from_this<aggregator_something>
{
        aggregator_something(size_t n)
        {
                assert( count_ > 0 && "preconditon failed");
                ptr_ = std::make_shared<equity_breakdown_matrix_aggregator>(n);
                fut_ = promise_.get_future();
        }
        void append( equity_breakdown const& breakdown , std::vector<int> const& mat){
                assert( count_ > 0 && "preconditon failed");
                std::lock_guard<std::mutex> lock(mtx_);
                ptr_->append_matrix( breakdown, mat);
                if( start_ && --count_ == 0 ){
                        at_finish_();
                }

                std::cout << *ptr_ << "\n";
        }
        auto get_future(){ return fut_; }
        void push_post_hook(std::function<void()> f){
                post_hook_.push_back(std::move(f));
        }
        void finalize(){
                start_ =true;
                if( count_ == 0 ){
                        at_finish_();
                }
        }
        void up(){ ++count_; }
private:
        void at_finish_(){
                std::cout << "at_finish_()\n";
                promise_.set_value( ptr_ );
                for( auto& f : post_hook_ )
                        f();
        }
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

        using future_or_now_t = boost::variant<
                equity_breakdown const*,
                std::shared_ptr<aggregator_something>
        >;
        future_or_now_t get_future_or_now_for_class_(boost::asio::io_service& io, holdem_class_vector const& vec){
                if( class_cache_ ){
                        class_cache_->lock();
                        auto ptr = class_cache_->lookup( vec );
                        class_cache_->unlock();
                        if( !! ptr){
                                std::cout << "CACHE HIT\n";
                                return ptr;
                        }
                }
                auto stdhands = vec.to_standard_form_hands(); 
                auto agg = std::make_shared<aggregator_something>(vec.size());
                for( auto const& stdhand : stdhands ){

                        auto const& hand_perm = std::get<0>(stdhand);
                        auto const& hand_vec  = std::get<1>(stdhand);

                        agg->up();
                        io.post( [agg, hand_perm, hand_vec]()mutable{
                                 auto const& eval = equity_evaluator_factory::get("principal");
                                 auto ret = eval.evaluate(hand_vec);
                                 agg->append( *ret, hand_perm);
                        });
                }
                return agg;
        }

        
        std::shared_future<
                std::shared_ptr<equity_breakdown>
        >
        foo(boost::asio::io_service& io, holdem_class_range_vector const& vec){

                auto range_agg = std::make_shared<aggregator_something>(vec.size());

                for( auto const& cp : vec.to_class_standard_form() ){

                        auto const& class_mat = std::get<0>(cp);
                        auto const& class_vec = std::get<1>(cp);
                
                        auto class_ret = get_future_or_now_for_class_(io, class_vec);
                        auto cache_hit = boost::get< equity_breakdown const* >(&class_ret);
                        if( cache_hit != nullptr ){
                                range_agg->append(**cache_hit, class_mat);
                                continue;
                        }
                        auto agg = boost::get<std::shared_ptr<aggregator_something> >(class_ret);
                        range_agg->up();
                        agg->push_post_hook(
                                [range_agg, agg, class_mat]()
                                {
                                        range_agg->append(*agg->get_future().get(), class_mat);
                                }
                        );
                        agg->push_post_hook(
                                [agg,this,class_vec]()mutable
                                {
                                        if( class_cache_ ){
                                                std::unique_lock<holdem_class_eval_cache> lock(*class_cache_);
                                                class_cache_->commit(class_vec, *agg->get_future().get() );
                                        }
                                }
                        );
                        agg->finalize();
                }
                std::shared_future<std::shared_ptr<equity_breakdown> > fut(range_agg->get_future());
                range_agg->finalize();
                return fut;
        }

        std::shared_future<
                std::shared_ptr<equity_breakdown>
        >
        foo(boost::asio::io_service& io, holdem_class_vector const& vec){

                auto stdvec = vec.to_standard_form();
                auto const& perm     = std::get<0>(stdvec);
                auto const& perm_vec = std::get<1>(stdvec);

                auto class_ret = get_future_or_now_for_class_(io, perm_vec);
                auto cache_hit = boost::get< equity_breakdown const* >(&class_ret);
                if( cache_hit != nullptr ){
                        std::promise<std::shared_ptr<equity_breakdown> > aux;
                        aux.set_value(
                                std::make_shared<equity_breakdown_matrix>(
                                        **cache_hit,
                                        perm));
                        return aux.get_future();
                }


                auto agg = boost::get<std::shared_ptr<aggregator_something> >(class_ret);

                auto ret_prom = std::make_shared<std::promise<std::shared_ptr<equity_breakdown> > >();
                std::shared_future<std::shared_ptr<equity_breakdown> > fut(ret_prom->get_future());
                
                // set return value
                agg->push_post_hook(
                        [ret_prom, perm,agg]()mutable
                        {
                                ret_prom->set_value(
                                        std::make_shared<equity_breakdown_permutation_view>(
                                                agg->get_future().get(),
                                                perm));

                        }
                );
                // add to cache
                agg->push_post_hook(
                        [agg,this,perm_vec]()mutable
                        {
                                if( class_cache_ ){
                                        std::unique_lock<holdem_class_eval_cache> lock(*class_cache_);
                                        class_cache_->commit(perm_vec, *agg->get_future().get() );
                                }
                        }
                );

                agg->finalize();

                return fut;
        }
        void inject_cache(std::shared_ptr<holdem_class_eval_cache> ptr){
                class_cache_ = ptr;
        }
private:
        std::shared_ptr<holdem_class_eval_cache> class_cache_;
};
} // ps

#endif // PS_EVAL_EQUITY_FUTURE_EVAL_H

#if 0
int main(){
        boost::timer::auto_cpu_timer at;
        holdem_class_range_vector players;
        //holdem_class_vector players;

        #if 0
        // Hand 0:  30.845%   28.23%  02.61%      3213892992  297127032.00   { TT+, AQs+, AQo+ }
        // Hand 1:  43.076%   40.74%  02.33%      4637673516  265584984.00   { QQ+, AKs, AKo }
        // Hand 2:  26.079%   25.68%  00.40%      2923177728   45324924.00   { TT }
        players.push_back(" TT+, AQs+, AQo+ ");
        players.push_back(" QQ+, AKs, AKo ");
        players.push_back("TT");
        #elif 1
        // Hand 0:  20.371%   17.93%  02.44%     30818548800  4197937635.60   { AKo }
        // Hand 1:  16.316%   15.71%  00.60%     27008469180  1037843085.60   { KQs }
        // Hand 2:  12.791%   12.43%  00.36%     21362591328  624688383.60   { Q6s-Q4s }
        // Hand 3:  12.709%   10.43%  02.28%     17933453280  3911711199.60   { ATo+ }
        // Hand 4:  37.813%   37.72%  00.09%     64837718124  160168887.60   { TT-77 }
        players.push_back(" AKo ");
        players.push_back(" KQs ");
        players.push_back(" Q6s-Q4s ");
        players.push_back(" ATo+ ");
        players.push_back(" TT-77 ");
        #elif 1
        // Hand 0:  59.954%   59.49%  00.46%     19962172212  155993282.00   { KK+, AKs }
        // Hand 1:  15.769%   15.22%  00.55%      5106357048  185284538.00   { 55+, A2s+, K5s+, Q8s+, J8s+, T8s+, 98s, A7o+, K9o+, Q9o+, JTo }
        // Hand 2:  24.277%   24.07%  00.21%      8076681624   69710696.00   { QQ }
        players.push_back("KK+, AKs ");
        players.push_back(" 55+, A2s+, K5s+, Q8s+, J8s+, T8s+, 98s, A7o+, K9o+, Q9o+, JTo ");
        players.push_back("QQ");
        #else
        // Hand 0:  81.547%   81.33%  00.22%        50134104     133902.00   { AA }
        // Hand 1:  18.453%   18.24%  00.22%        11241036     133902.00   { QQ }
        players.push_back("AA");
        players.push_back("QQ");
        #endif

        auto cache = std::make_shared<holdem_class_eval_cache>();
        //cache->load("cache.bin");
        cache->display();

        boost::asio::io_service io;
        //for(int i=0;i!=3;++i){
                equity_future_eval b;
                b.inject_cache(cache);
                auto ret = b.foo(io, players);
                std::vector<std::thread> tg;
                for(size_t i=0;i!=std::thread::hardware_concurrency();++i){
                        tg.emplace_back( [&](){ io.run(); } );
                }
                for( auto& t : tg)
                        t.join();
                std::cout << *ret.get() << "\n";
        //}
        std::cout << "cache\n";

        //cache->display();
        //cache->save("cache.bin");
        

}
#endif
