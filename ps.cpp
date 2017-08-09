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
#include "ps/eval/equity_future.h"

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
        
        equity_future ef;
        processor proc;
        
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        std::vector< std::vector<int>, result_t> items;

        for( auto hv : players.get_hand_vectors()){
                auto p =  permutate_for_the_better(players) ;
                auto& perm = std::get<0>(p);
                auto const& perm_players = std::get<1>(p);
                auto fut = ef.calculate(proc, hv);
                items.emplace_back(perm, fut);
        }
        proc.run();
        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
        for( auto& t : world ){
                result->append(
                        *std::make_shared<equity_breakdown_permutation_view>(
                                std::get<1>(t).get(),
                                std::get<0>(t)));
        }
        std::cout << *result << "\n";
        std::cout << *result << "\n";
        std::cout << *result << "\n";


}
