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

#ifdef NOT_DEFINED
#include "ps/simulation.h"

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

        auto ctx = maker.compile();


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

TEST_F( simulation, _3){
        /*
                +----+------+-----------+---------+
                |Hand|Equity|   Wins    |  Ties   |
                +----+------+-----------+---------+
                | 55 |29.36%|115,460,928|1,331,496|
                |ako |38.25%|150,538,824|1,331,496|
                |89s |32.40%|127,445,904|1,331,496|
                +----+------+-----------+---------+

                +----+------+----------+-------+
                |Hand|Equity|   Wins   | Ties  |
                +----+------+----------+-------+
                |Ako |45.38%|55,669,464|562,284|
                | 55 |54.62%|67,054,140|562,284|
                +----+------+----------+-------+
        */

        ps::simulation_calc sc;
        simulation_context_maker maker;

        maker.begin_player();
        maker.add("55");
        maker.end_player();
        

        maker.begin_player();
        maker.add("AKo");
        maker.end_player();
        
        maker.begin_player();
        maker.add("89s");
        maker.end_player();

        maker.debug();

        auto ctx = maker.compile();


        sc.run(ctx);
        
        auto iter = std::begin(ctx.get_players());
        auto end =  std::end(  ctx.get_players());

        auto _55 = *iter++;
        auto _AKo = *iter++;
        auto _89s = *iter++;
        
        EXPECT_EQ( 115'460'928, _55.wins());
        EXPECT_EQ(   1'331'496 , _55.draws());
        EXPECT_NEAR( 0.2936,    _55.equity(), 1e-3);
        
        EXPECT_EQ( 150'538'824, _AKo.wins());
        EXPECT_EQ(    1'331'496 , _AKo.draws());
        EXPECT_NEAR( 0.3825,    _AKo.equity(), 1e-3);
        
        EXPECT_EQ( 127'445'904, _89s.wins());
        EXPECT_EQ(    1'331'496 , _89s.draws());
        EXPECT_NEAR( 0.3240,    _89s.equity(), 1e-3);
}

#endif
