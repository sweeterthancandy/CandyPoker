#include <gtest/gtest.h>

using namespace ps;


TEST( equity_calc_, _){
        ps::equity_calc eq;
        ps::equity_context ctx;

        ctx.add_player("Askd");
        ctx.add_player("ThTc");

        eq.eval( ctx );

        for( auto const& p : ctx.get_players()){

        }



}
