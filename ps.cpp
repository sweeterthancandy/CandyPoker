#include <iostream>
#include <string>
#include <type_traits>
#include <chrono>
#include "ps/support/processor.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/class_equity_evaluator_cache.h"
#include "ps/eval/class_range_equity_evaluator.h"
#include "ps/eval/class_equity_future.h"
#include "ps/detail/cross_product.h"

using namespace ps;


#if 1
struct class_range_equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        class_range_equity_future()
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
        result_t schedual_group(support::processor::process_group& pg, holdem_class_range_vector const& players){
                std::cout << "schedual_group(" << players << "\n";
                std::vector<result_t> params;
                for( auto const& cv : players.get_cross_product()){
                        auto sub = std::make_unique<support::processor::process_group>();
                        params.push_back( schedual_group(*sub, cv) );
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
        mutable equity_future ef_;
        std::map< holdem_hand_vector, result_t > m_;
};
#endif

int main(){
        holdem_class_range_vector players;
        #if 1
        players.push_back("99+, AJs+, AQo+");
        players.push_back("55");
        #endif
        // Hand 0: 	81.547%  	81.33% 	00.22% 	      50134104 	   133902.00   { AA }
        // Hand 1: 	18.453%  	18.24% 	00.22% 	      11241036 	   133902.00   { QQ }
        #if 0
        players.push_back("AA");
        players.push_back("QQ");
        #endif

        #if 0
        PRINT(players);
        for( auto const& p : players.get_cross_product()){
                for( auto const& hv : p.get_hand_vectors()){
                        PRINT(hv);
                }
        }
        #endif
        
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












