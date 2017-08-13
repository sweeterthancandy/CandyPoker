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

using namespace ps;


struct equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        equity_future()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {}
        result_t schedual_group(support::processor::process_group& pg, holdem_hand_vector const& players){
                std::cout << "schedual_group(" << players << "\n";

                if( players.is_standard_form() ){
                        auto iter = m_.find(players);
                        if( iter != m_.end() ){
                                return iter->second;
                        }
                        auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                                [players,this](){
                                return impl_->evaluate(players);
                        });
                        m_.emplace(players, std::move(task->get_future()));

                        // yuk
                        auto w = [task]()mutable{
                                (*task)();
                        };
                        pg.push(std::move(w));
                        return schedual_group(pg, players);
                } else {
                        auto p =  permutate_for_the_better(players) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);

                        auto perm_ret = schedual_group(pg, perm_players);
                        auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                                [perm,perm_ret](){
                                return 
                                        std::make_shared<equity_breakdown_permutation_view>(
                                                perm_ret.get(),
                                                perm
                                        );
                        });
                        result_t ret = task->get_future();
                        // yuk
                        auto w = [task]()mutable{
                                (*task)();
                        };
                        pg.push(std::move(w));
                        return std::move(ret);
                }


        }
private:
        equity_evaluator* impl_;
        // only cache those in standard form
        std::map< holdem_hand_vector, result_t > m_;
};

struct class_equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        class_equity_future()
        {
        }
        result_t schedual_group(support::processor::process_group& pg, holdem_class_vector const& players){
                std::cout << "schedual_group(" << players << "\n";
                
                std::vector< std::tuple< std::vector<int>, result_t > > items;

                for( auto hv : players.get_hand_vectors()){
                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);
                        auto fut = ef_.schedual_group(pg, perm_players);
                        items.emplace_back(perm, fut);
                }
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [n_=players.size(),items_=std::move(items)](){
                        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(n_);
                        for( auto& t : items_ ){
                                result->append(
                                        *std::make_shared<equity_breakdown_permutation_view>(
                                                std::get<1>(t).get(),
                                                std::get<0>(t)));
                        }
                        return result;
                });
                result_t fut = task->get_future();
                m_.emplace(players, fut);
                pg.sequence_point();
                pg.push([task]()mutable{ (*task)(); });
                return fut;
        }
private:
        ::equity_future ef_;
        std::map< holdem_hand_vector, result_t > m_;
};

struct class_range_equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        #if 0
        result_t schedual_group(support::processor::process_group& pg, holdem_class_range_vector const& players){
                std::cout << "schedual_group(" << players << "\n";

                std::vector< std::tuple< std::vector<int>, result_t > > items;

                for( auto const& cv : players.get_cross_product()){
                        auto sub = std::make_unique<support::processor::process_group>();
                        items.push_back( cef_.schedual_group(*sub, cv_perm) );
:w
                        pg.push(std::move(sub));
                }
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [n_=players.size(),items_=std::move(items)](){
                        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(n_);
                        for( auto& t : items_ ){
                                result->append(
                                        *std::make_shared<equity_breakdown_permutation_view>(
                                                std::get<1>(t).get(),
                                                std::get<0>(t)));
                        }
                        return result;
                });
                result_t fut = task->get_future();
                pg.sequence_point();
                pg.push([task]()mutable{ (*task)(); });
                return fut;
        }
        #endif
        result_t schedual_group(support::processor::process_group& pg, holdem_class_range_vector const& players){
                std::cout << "schedual_group(" << players << "\n";
                std::vector<result_t> params;
                for( auto const& cv : players.get_cross_product()){
                        auto sub = std::make_unique<support::processor::process_group>();
                        params.push_back( cef_.schedual_group(*sub, cv) );
                        pg.push(std::move(sub));
                }
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [_n=players.size(),_params=std::move(params)](){
                                std::cout << "in task\n";
                                PRINT(_params.size());
                                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(_n);
                                for( auto& item : _params ){
                                        result->append(*item.get());
                                }
                                return std::move(result);
                        }
                );
                result_t fut = task->get_future();
                pg.sequence_point();
                pg.push([task]()mutable{ (*task)(); });
                return fut;
        }
private:
        ::class_equity_future cef_;
};

int main(){
        boost::timer::auto_cpu_timer at;
        holdem_class_range_vector players;
        #if 0
        players.push_back(" TT+, AQs+, AQo+ ");
        players.push_back(" QQ+, AKs, AKo ");
        players.push_back("TT");
        #endif
        // Hand 0:  81.547%   81.33%  00.22%        50134104     133902.00   { AA }
        // Hand 1:  18.453%   18.24%  00.22%        11241036     133902.00   { QQ }
        players.push_back("AA");
        players.push_back("QQ");
        
        #if 1
        class_range_equity_future cef;
        
        support::processor proc;
        for( size_t i=0; i!= std::thread::hardware_concurrency();++i){
                proc.spawn();
        }
        auto g = std::make_unique<support::processor::process_group>();
        //auto ret = cef.schedual_group(*g, players.get_cross_product().front());
        auto ret = cef.schedual_group(*g, players);
        proc.accept(std::move(g));
        proc.join();
        std::cout << *ret.get() << "\n";
        #endif
}

#if 0
int main(){
        support::processor proc;
        for( size_t i=0; i!= std::thread::hardware_concurrency();++i){
                proc.spawn();
        }
        holdem_class_vector players;
        players.push_back("AKs");
        players.push_back("KQs");
        players.push_back("QJs");
        class_equity_future ef;
        auto pg = std::make_unique<support::processor::process_group>();
        auto ret = ef.schedual_group(*pg, players);
        proc.accept(std::move(pg));
        proc.join();
        std::cout << *(ret.get()) << "\n";
        #if 0
        holdem_class_vector players;
        players.push_back("99");
        players.push_back("55");
        class_equity_evaluator_cache ec;

        ec.load("cache.bin");
        std::cout << *ec.evaluate(players) << "\n";
        ec.save("cache.bin");
        #endif
        #if 0
        holdem_class_vector players;
        players.push_back("AA");
        players.push_back("QQ");
        class_equity_evaluator_cache ec;
        std::cout << *ec.evaluate(players) << "\n";
        #endif

}
#endif


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


int processor_test0(){
        processor proc;
        #if 1
        for(int i=0;i!=100;++i)
                proc.spawn();
                #endif
        proc.spawn();
        #if 1
        auto g = std::make_unique<processor::process_group>();
        g->push( make_proto("a"));
        g->push( make_proto("b"));
        g->sequence_point();
        g->push( make_proto("c"));
        auto h = std::make_unique<processor::process_group>();
        h->push(std::move(g));
        //h->sequence_point();
        h->push([]{std::cout << "Finished\n"; });

        proc.accept(std::move(h));
        #endif
        proc.join();
}
#endif 












