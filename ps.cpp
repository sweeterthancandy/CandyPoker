#include <iostream>
#include "ps/base/hand_vector.h"
#include "ps/base/frontend.h"
#include "ps/eval/class_equity_evaluator.h"

using namespace ps;


int main(){
        holdem_class_vector players;
        players.push_back(holdem_class_decl::parse("A2o"));
        players.push_back(holdem_class_decl::parse("88"));
        players.push_back(holdem_class_decl::parse("QJs"));

        auto const& ec = class_equity_evaluator_factory::get("proc");
        std::cout << *ec.evaluate(players) << "\n";

        PRINT( players );


}
