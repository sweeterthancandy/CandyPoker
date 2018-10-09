#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <map>
#include <codecvt>
#include <type_traits>
#include <functional>
#include <type_traits>
#include <functional>
#include <iostream>
#include <future>

#include <boost/range/algorithm.hpp>
#include "ps/detail/visit_combinations.h"

#include "ps/base/range.h"
#include "ps/base/card_vector.h"
#include "ps/base/frontend.h"
#include "ps/base/algorithm.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/evaluator.h"
#include "ps/eval/rank_world.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown.h"
#include "ps/eval/range_equity_evaluator.h"

using namespace ps;

struct computational_work{
};

struct execution_strategy{
};


int main(){
        holdem_class_vector players;
        players.push_back(holdem_class_decl::parse("A2o"));
        players.push_back(holdem_class_decl::parse("88"));
        players.push_back(holdem_class_decl::parse("QJs"));
        
        std::map<
                holdem_hand_vector,
                std::shared_future<std::shared_ptr<equity_breakdown> >
        > cache;
        std::list<
                std::tuple<
                        std::vector<int>,
                        std::shared_future<std::shared_ptr<equity_breakdown> >
                >
        > world;

        auto const& eval = equity_evaluator_factory::get("principal");

        for( auto hv : players.get_hand_vectors()){
                auto p =  permutate_for_the_better(hv) ;
                auto& perm = std::get<0>(p);
                auto const& perm_players = std::get<1>(p);
                
                auto iter = cache.find(perm_players);
                if( iter == cache.end()){

                        std::packaged_task<std::shared_ptr<equity_breakdown>()> task(
                                [perm_players,&eval](){
                                return eval.evaluate(perm_players);
                        });
                        cache.emplace(perm_players, std::move(task.get_future()));
                        iter = cache.find(perm_players);
                        PRINT(iter == cache.end());
                        std::thread(std::move(task)).detach();
                }

                world.emplace_back( perm, iter->second );
        }
        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
        for( auto& t : world ){
                                std::get<1>(t).get();
                                std::get<1>(t).get();
                result->append(
                        *std::make_shared<equity_breakdown_permutation_view>(
                                std::get<1>(t).get(),
                                std::get<0>(t)));
        }
        std::cout << *result << "\n";
        std::cout << *result << "\n";
        std::cout << *result << "\n";


}
