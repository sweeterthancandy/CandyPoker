#include <iostream>
#include <string>
#include <type_traits>
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/class_range_equity_evaluator.h"

using namespace ps;




int main(){
        holdem_class_range_vector players;
        players.emplace_back("99+, AJs+, AQo+");
        players.emplace_back("55");
        auto const& ec = class_range_equity_evaluator_factory::get("principal");
        std::cout << *ec.evaluate(players) << "\n";

}
