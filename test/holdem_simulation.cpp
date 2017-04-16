
#include <gtest/gtest.h>

#include "ps/holdem/simulation.h"

using namespace ps;

struct simulation : testing::Test{
protected:
        ps::simulation_calc sc;
};

TEST_F( simulation, _){
        simulation_context_maker maker;

        maker.begin_player();
        maker.add("55");
        maker.end_player();

        maker.begin_player();
        maker.add("AK");
        maker.end_player();

        maker.debug();

        auto ctx{maker.compile()};


        sc.run(ctx);
        
        auto iter = std::begin(ctx.get_players());
        auto end =  std::end(  ctx.get_players());

        auto _55 = *iter++;
        auto _AK = *iter++;
        
        EXPECT_EQ( 88'311'564 , _55.wins());
        EXPECT_EQ(    758'292 , _55.draws());
        EXPECT_EQ( 164'381'184, _55.sigma());
        EXPECT_NEAR( 0.5395,    _55.equity(), 1e-3);
        
        EXPECT_EQ( 75'311'328, _AK.wins());
        EXPECT_EQ(    758'292 , _AK.draws());
        EXPECT_EQ( 164'381'184, _AK.sigma());
        EXPECT_NEAR( 0.4605,    _AK.equity(), 1e-3);
}
