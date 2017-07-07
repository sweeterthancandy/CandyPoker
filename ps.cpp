#include "ps/calculator_detail.h"
#include "ps/heads_up.h"

#include <boost/timer/timer.hpp>

using namespace ps;
using namespace ps::detail;


int main(){
        equity_calc_detail ecd;
        equity_cacher ec;
        class_equity_cacher cec{ec};
        ec.load("cache.bin");
        basic_calculator_N<
                basic_detailed_calculation_decl<2>,
                2 > other_ec{&ecd};

        PRINT(  ec.visit_boards( std::vector<ps::holdem_id>{
                                 holdem_hand_decl::parse("AdKs").id(),
                                 holdem_hand_decl::parse("TsTh").id()} ) );
        PRINT(  ec.visit_boards( std::vector<ps::holdem_id>{
                                 holdem_hand_decl::parse("TsTh").id(),
                                 holdem_hand_decl::parse("AdKs").id()
                                 } ) );
        PRINT( other_ec.calculate( std::array<ps::holdem_id, 2>{
                                 holdem_hand_decl::parse("AdKs").id(),
                                 holdem_hand_decl::parse("TsTh").id()
                                 } ) );
        PRINT( other_ec.calculate( std::array<ps::holdem_id, 2>{
                                 holdem_hand_decl::parse("TsTh").id(),
                                 holdem_hand_decl::parse("AdKs").id()
                                 } ) );

}
