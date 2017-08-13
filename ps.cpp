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
        std::vector<holdem_class_strategy> player_strategy;
        for(size_t i=0;i!=3;++i){
                player_strategy.emplace_back();
        }
        player_strategy.back().display();
        
}
