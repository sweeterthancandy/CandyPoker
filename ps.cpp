#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/sim/holdem_class_strategy.h"


/*
                Strategy in the form 
                        vpip/fold
 */

using namespace ps;

int main(){
        #if 0
        std::vector<holdem_class_strategy> player_strategy;
        for(size_t i=0;i!=3;++i){
                player_strategy.emplace_back();
        }
        player_strategy.back().display();
        #endif

        auto& eval = class_equity_evaluator_factory::get("cached");
        auto& cache = holdem_class_eval_cache_factory::get("main");
        cache.load("result.bin");
        eval.inject_cache( std::shared_ptr<holdem_class_eval_cache>(&cache, [](auto){}));
        holdem_class_vector vec;
        vec.push_back("AA");
        vec.push_back("KK");
        std::cout << *eval.evaluate(vec) << "\n";
        cache.save("better_result.bin");

        
}
