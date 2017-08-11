#include <iostream>
#include <string>
#include <type_traits>
#include <chrono>
#include "ps/base/holdem_class_vector.h"
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/class_equity_evaluator_cache.h"
#include "ps/eval/class_range_equity_evaluator.h"

using namespace ps;

struct class_equity_future{
        class_equity_future()
                : impl_{ &equity_evaluator_factory::get("principal") }
        {
        }
        std::shared_ptr<equity_breakdown> evaluate(holdem_class_vector const& players)const{
                support::processor proc;
                for( size_t i=0; i!= std::thread::hardware_concurrency();++i)
                        proc.spawn();
                
                using result_t = std::shared_future<
                        std::shared_ptr<equity_breakdown>
                >;
                std::vector< std::tuple< std::vector<int>, result_t > > items;

                for( auto hv : players.get_hand_vectors()){
                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);
                        auto fut = ef_.schedual(proc, perm_players);
                        items.emplace_back(perm, fut);
                }
                proc.join();
                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
                for( auto& t : items ){
                        result->append(
                                *std::make_shared<equity_breakdown_permutation_view>(
                                        std::get<1>(t).get(),
                                        std::get<0>(t)));
                }
                return result;
        }
private:
        equity_evaluator const* impl_;
        mutable equity_future ef_;
};

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

int main(){
        #if 1
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
