#include "ps/calculator_detail.h"
#include "ps/heads_up.h"

#include <boost/timer/timer.hpp>

using namespace ps;
using namespace ps::detail;


void test0(){
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

void test1(){
        equity_calc_detail ecd;

        equity_cacher ec;
        ec.load("cache.bin");

        basic_calculator_N<
                basic_detailed_calculation_decl<2>,
                2 > other_ec{&ecd};
        size_t count{0};
        boost::timer::auto_cpu_timer at;
        other_ec.load("new_cache.bin");
        for(ps::holdem_id i=0; i!= holdem_hand_decl::max_id;++i){
                for(ps::holdem_id j=0; j!= holdem_hand_decl::max_id;++j){
                        if( disjoint(holdem_hand_decl::get(i),holdem_hand_decl::get(j)) ){
                                auto left{ other_ec.calculate(
                                        std::array<ps::holdem_id, 2>{ i, j } ) };
                                #if 1
                                auto right{ ec.visit_boards(
                                        std::vector<ps::holdem_id>{ i, j } ) };
                                        #endif
                                #if 1
                                PRINT(right);
                                PRINT(left);
                                PRINT( std::fabs( left.player(0).equity() - right.equity() ) );
                                #endif

                                #if 0
                                if( ++count % 100 == 0 ){
                                        std::cerr << "saving\n";
                                        other_ec.save("new_cache.bin");
                                        other_ec.save("new_cache.binn");
                                        std::cerr << "done\n";
                                }
                                #endif
                                PRINT(count);
                                if( ++count == 500 )
                                        goto cleanup;

                        }
                }
        }
cleanup:
        other_ec.save("new_cache.bin");
}

int main(){
        test1();
}
