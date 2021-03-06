/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include <gtest/gtest.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"

#include "ps/heads_up.h"
#include "ps/eval/calculator.h"

using namespace ps;

#if 0

struct calculator_ : testing::Test{
        calculator_()
        {
                calc.load("newcache.bin");
        }
        calculater calc;
};


// this is super slow
#if 0
TEST_F( equity_calc_, aggregation){
        double sigma{0.0};
        hu_fresult_t agg;
        for(holdem_id x{0};x!=holdem_hand_decl::max_id;++x){
                for(holdem_class_id y{0};y!=holdem_hand_decl::max_id;++y){
                        auto const& ret =  ec.visit_boards( std::vector<ps::holdem_id>{ x, y } ) ;
                        agg.append(ret);
                        sigma += ret.equity() * holdem_hand_decl::prob(x,y);
                }
        }
        EXPECT_NEAR( agg.equity(), .5, 1e-5 );
        EXPECT_NEAR( sigma       , .5, 1e-5 );
}
#endif

        /*
        Hand 0: 	65.771%  	65.55% 	00.22% 	      26939748 	    89034   { 77 }
        Hand 1: 	34.229%  	34.01% 	00.22% 	      13977480 	    89034   { A5s }

        Hand 0: 	50.000%  	02.17% 	47.83% 	        223296 	  4913616   { TT }
        Hand 1: 	50.000%  	02.17% 	47.83% 	        223296 	  4913616   { TT }

        Hand 0: 	32.860%  	22.46% 	10.40% 	      41543220 	 19223502   { J7o }
        Hand 1: 	67.140%  	56.75% 	10.40% 	     104938608 	 19223502   { J8o }

        Hand 0: 	30.222%  	26.88% 	03.34% 	       5523852 	   685974   { 96s }
        Hand 1: 	69.778%  	66.44% 	03.34% 	      13651848 	   685974   { T9s }

        Hand 0: 	45.383%  	45.15% 	00.23% 	      55669464 	   281142   { AKo }
        Hand 1: 	54.617%  	54.39% 	00.23% 	      67054140 	   281142   { 55 }

        */
TEST_F( calculator_, _){
        struct regression_result{
                regression_result()=default;
                regression_result(std::string const& h, std::string const& v, double e):
                        hero{h}, villian{v}, equity{e}
                {}
                std::string hero;
                std::string villian;
                double equity;
        };

        // just take some results from pokerstove
        std::vector<regression_result> ticker = {
                {  "77", "A5s",  0.65771 },
                {  "TT",  "TT",  0.50000 },
                { "J7o", "J8o",  0.32860 },
                { "96s", "T9s",  0.30222 },
                { "AKo",  "55",  0.45383 }
        };
        for( auto const& r : ticker){
                auto const& ret{ calc.calculate_class_equity( 
                                                   ps::holdem_class_decl::parse( r.hero ),
                                                   ps::holdem_class_decl::parse( r.villian ) ) };
                EXPECT_NEAR( r.equity, ret.player(0).equity(), 1e-3 );
        }
}

TEST_F( calculator_, aggregation){
        struct regression_result{
                regression_result()=default;
                regression_result(std::string const& h, std::string const& v, double e):
                        hero{h}, villian{v}, equity{e}
                {}
                std::string hero;
                std::string villian;
                double equity;
        };

        // just take some results from pokerstove
        std::vector<regression_result> ticker = {
                {  
                        "100%",
                        "100%",
                        0.5
                },
                {  
                        "ATo+ KTo+ QTo+ JTo",
                        "100%",
                        0.60847
                },
                { 
                        "22+",
                        "100%",
                        0.68625
                },
                {  
                        "A2s+, K2s+, Q2s+, J2s+, T2s+, 92s+, 82s+, 72s+, 62s+, 52s+, 42s+, 32s ",
                        "100%",
                        0.50899
                },
                {
                        " TT+, ATs+, KTs+, QTs+, JTs, ATo+, KTo+, QTo+, JTo ",
                        " A2s+, K2s+, Q2s+, J2s+, T2s+, 92s+, 82s+, 72s+, 62s+, 52s+, 42s+, 32s ",
                        0.62785
                },
                {
                        " TT+, ATs+, KTs+, QTs+, JTs, ATo+, KTo+, QTo+, JTo ",
                        " 22+, A2s+, K2s+, Q2s+, J2s+, T2s+, 92s+, 82s+, 72s+, 62s+, 52s+, 42s+, 32s ",
                        0.59372
                },
                {
                        " TT+, ATs+, KTs+, QTs+, JTs, ATo+, KTo+, QTo+, JTo ",
                        " A2o+, K2o+ ",
                        0.57313
                }
                #if 0
                {
                        "",
                        "",
                        0.0
                },
                #endif

        };
        for( auto const& r : ticker){
                double sigma{0};
                size_t factor{0};
                tree_range root{ std::vector<frontend::range>{frontend::parse(r.hero),
                                                              frontend::parse(r.villian)} };
                aggregator agg;
                for( auto const& c : root.children ){

                        ASSERT_EQ( c.opt_cplayers.size(), 2 );
                        
                        auto ret =  calc.calculate_class_equity( c.opt_cplayers ) ;
                        agg.append(ret);

                        size_t weight{ holdem_class_decl::weight( c.opt_cplayers[0], c.opt_cplayers[1]) };
                        sigma += ret.player(0).equity() * weight;
                        factor += weight;
                }
                double equity{ sigma / factor };
                //PRINT_SEQ((agg.equity())(r.equity)(equity)(std::fabs(agg.equity()-r.equity)));
                EXPECT_NEAR( agg.make_view().player(0).equity(), r.equity, 1e-3 );
                EXPECT_NEAR( equity, r.equity, 1e-3 );
        }
}

TEST_F( calculator_, range_vs_range ){
}
#endif

#if NOT_DEFINED

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
        
        eq.run(result, players, board, dead);

        auto p =  result.proxy() ;

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

#endif

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


