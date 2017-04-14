#include <gtest/gtest.h>

#include "ps/holdem/equity_calc.h"

using namespace ps;

struct equity_calc_ : testing::Test{
protected:
        ps::equity_calc eq;
};

TEST_F( equity_calc_, _AhKs_2s2c){
        ps::equity_context ctx;

        ctx.add_player("AhKh");
        ctx.add_player("2s2c");

        eq.run( ctx );

        EXPECT_EQ( 2, ctx.get_players().size() );

        auto iter = std::begin(ctx.get_players());
        auto end =  std::end(  ctx.get_players());

        auto _AhKh = *iter++;
        auto _2s2c = *iter++;

        ASSERT_EQ( iter, end );

        EXPECT_EQ( 852'207 , _AhKh.wins());
        EXPECT_EQ( 10'775  , _AhKh.draws());
        EXPECT_NEAR( 0.5008, _AhKh.equity(), 1e-3);
        
        EXPECT_EQ( 849'322 , _2s2c.wins());
        EXPECT_EQ( 10'775  , _2s2c.draws());
        EXPECT_NEAR( 0.4992, _2s2c.equity(), 1e-3);
}

TEST_F( equity_calc_, _AhKs_2s2c_5c6c){
        ps::equity_context ctx;

        ctx.add_player("AhKh");
        ctx.add_player("2s2c");
        ctx.add_player("5c6c");

        eq.run( ctx );

        EXPECT_EQ( 3, ctx.get_players().size() );

        auto iter = std::begin(ctx.get_players());
        auto end =  std::end(  ctx.get_players());

        auto _AhKh = *iter++;
        auto _2s2c = *iter++;
        auto _5c6c = *iter++;

        ASSERT_EQ( iter, end );

        EXPECT_EQ( 574'928 , _AhKh.wins());
        EXPECT_EQ( 7'155   , _AhKh.draws());
        EXPECT_NEAR( 0.4212, _AhKh.equity(), 1e-3);
        
        EXPECT_EQ( 343'287 , _2s2c.wins());
        EXPECT_EQ( 7'155   , _2s2c.draws());
        EXPECT_NEAR( 0.2522, _2s2c.equity(), 1e-3);
        
        EXPECT_EQ( 445'384 , _5c6c.wins());
        EXPECT_EQ( 7'155   , _5c6c.draws());
        EXPECT_NEAR( 0.3267, _5c6c.equity(), 1e-3);
}


TEST_F( equity_calc_, _AhKs_2s2c_5c6c__with_board){
        ps::equity_context ctx;

        ctx.add_player("AhKh");
        ctx.add_player("2s2c");
        ctx.add_player("5c6c");

        ctx.add_board("8d9djs");

        eq.run( ctx );

        EXPECT_EQ( 3, ctx.get_players().size() );

        auto iter = std::begin(ctx.get_players());
        auto end =  std::end(  ctx.get_players());

        auto _AhKh = *iter++;
        auto _2s2c = *iter++;
        auto _5c6c = *iter++;

        ASSERT_EQ( iter, end );

        EXPECT_EQ( 244 , _AhKh.wins());
        EXPECT_EQ( 16   , _AhKh.draws());
        EXPECT_NEAR( 0.2761, _AhKh.equity(), 1e-3);
        
        EXPECT_EQ( 332 , _2s2c.wins());
        EXPECT_EQ( 16   , _2s2c.draws());
        EXPECT_NEAR( 0.3736, _2s2c.equity(), 1e-3);
        
        EXPECT_EQ( 311 , _5c6c.wins());
        EXPECT_EQ( 16   , _5c6c.draws());
        EXPECT_NEAR( 0.3503, _5c6c.equity(), 1e-3);
}
