#include <gtest/gtest.h>

#include "ps/equity_calc.h"
#include "ps/frontend.h"

using namespace ps;

struct equity_calc_ : testing::Test{
protected:
        std::vector<id_type> players;
        std::vector<id_type> board;
        std::vector<id_type> dead;
        ps::equity_calc eq;
        numeric::result_type result;
};

TEST_F( equity_calc_, _AhKs_2s2c){

        using namespace ps::literals;

        players.push_back( "5h6h"_h );
        players.push_back( "8sAh"_h );
        players.push_back( "2c2d"_h );
        players.push_back( "AcKc"_h );
        players.push_back( "8d9d"_h );
        players.push_back( "ThQh"_h );

        result = numeric::result_type{6};
        
        PRINT(result);
        
        eq.run( players, board, dead, result);

        auto p{ result.proxy() };

        auto _5h6h = result(0);
        auto _8sAh = result(1);
        auto _2c2d = result(2);
        auto _AcKc = result(3);
        auto _8d9d = result(4);
        auto _ThQh = result(5);

        EXPECT_EQ( 90'581  , _5h6h.wins());
        EXPECT_EQ( 906     , _5h6h.draws());
        EXPECT_NEAR( 0.1379, _5h6h.equity(), 1e-3);
        
        EXPECT_EQ( 31'473  , _8sAh.wins());
        EXPECT_EQ( 14'737  , _8sAh.draws());
        EXPECT_NEAR( 0.0586, _8sAh.equity(), 1e-3);

        EXPECT_EQ( 110'423 , _2c2d.wins());
        EXPECT_EQ(     906 , _2c2d.draws());
        EXPECT_NEAR( 0.1680, _2c2d.equity(), 1e-3);

        EXPECT_EQ( 164'648 , _AcKc.wins());
        EXPECT_EQ(   6'841 , _AcKc.draws());
        EXPECT_NEAR( 0.2550, _AcKc.equity(), 1e-3);

        EXPECT_EQ( 112'342 , _8d9d.wins());
        EXPECT_EQ(   8'802 , _8d9d.draws());
        EXPECT_NEAR( 0.1770, _8d9d.equity(), 1e-3);

        EXPECT_EQ( 133'804 , _ThQh.wins());
        EXPECT_EQ(     906 , _ThQh.draws());
        EXPECT_NEAR( 0.2036, _ThQh.equity(), 1e-3);

}

#if 0
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

        {
                auto _AhKh = ctx( frontend::_AhKh );
        
                EXPECT_EQ( 852'207 , _AhKh.wins());
                #if 0
                EXPECT_EQ( 10'775  , _AhKh.draws());
                EXPECT_NEAR( 0.5008, _AhKh.equity(), 1e-3);
                
                EXPECT_EQ( 849'322 , _2s2c.wins());
                EXPECT_EQ( 10'775  , _2s2c.draws());
                EXPECT_NEAR( 0.4992, _2s2c.equity(), 1e-3);
                #endif
        }
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

/*
 Hand   Equity  Wins    Ties
 5h6h   13.79%  90,581  906
 8sAh   5.86%   31,473  14,737
 2c2d   16.80%  110,423 906
 AcKc   25.50%  164,648 6,841
 8d9d   17.70%  112,342 8,802
 ThQh   20.36%  133,804 906
 */

TEST_F( equity_calc_, _5h6h_8sAh_2c2d_AcKc_8d9d_ThQh ){

        ps::equity_context ctx;

        ctx.add_player("5h6h");
        ctx.add_player("8sAh");
        ctx.add_player("2c2d");
        ctx.add_player("AcKc");
        ctx.add_player("8d9d");
        ctx.add_player("ThQh");

        eq.run( ctx );

        EXPECT_EQ( 6, ctx.get_players().size() );

        auto iter = std::begin(ctx.get_players());
        auto end =  std::end(  ctx.get_players());

        auto _5h6h = *iter++;
        auto _8sAh = *iter++;
        auto _2c2d = *iter++;
        auto _AcKc = *iter++;
        auto _8d9d = *iter++;
        auto _ThQh = *iter++;

        ASSERT_EQ( iter, end );

        EXPECT_EQ( 90'581  , _5h6h.wins());
        EXPECT_EQ( 906     , _5h6h.draws());
        EXPECT_NEAR( 0.1379, _5h6h.equity(), 1e-3);
        
        EXPECT_EQ( 31'473  , _8sAh.wins());
        EXPECT_EQ( 14'737  , _8sAh.draws());
        EXPECT_NEAR( 0.0586, _8sAh.equity(), 1e-3);

        EXPECT_EQ( 110'423 , _2c2d.wins());
        EXPECT_EQ(     906 , _2c2d.draws());
        EXPECT_NEAR( 0.1680, _2c2d.equity(), 1e-3);

        EXPECT_EQ( 164'648 , _AcKc.wins());
        EXPECT_EQ(   6'841 , _AcKc.draws());
        EXPECT_NEAR( 0.2550, _AcKc.equity(), 1e-3);

        EXPECT_EQ( 112'342 , _8d9d.wins());
        EXPECT_EQ(   8'802 , _8d9d.draws());
        EXPECT_NEAR( 0.1770, _8d9d.equity(), 1e-3);

        EXPECT_EQ( 133'804 , _ThQh.wins());
        EXPECT_EQ(     906 , _ThQh.draws());
        EXPECT_NEAR( 0.2036, _ThQh.equity(), 1e-3);
}
#endif

