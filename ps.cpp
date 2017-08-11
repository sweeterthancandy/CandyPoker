#include <iostream>
#include <string>
#include <type_traits>
#include "ps/base/holdem_class_vector.h"
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/class_equity_evaluator_cache.h"
#include "ps/eval/class_range_equity_evaluator.h"

using namespace ps;



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
