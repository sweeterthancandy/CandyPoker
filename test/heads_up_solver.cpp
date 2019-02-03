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

#include "ps/heads_up_solver.h"

using namespace ps;

struct hu_solver : testing::Test{
        hu_solver()
                :cec{ec}
        {
                ec.load("cache.bin");
                cec.load("hc_cache.bin");
        }
        equity_cacher ec;
        class_equity_cacher cec;
        double eff_stack = 10.0;
        double sb        = 0.5;
        double bb        = 1.0;
};

TEST_F(hu_solver, fold_fold){
        calc_context ctx = {
                &cec,
                hu_strategy{0.0},
                hu_strategy{0.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        EXPECT_NEAR( calc(ctx), -sb, 1e-5);
}
TEST_F(hu_solver, fold_call){
        calc_context ctx = {
                &cec,
                hu_strategy{0.0},
                hu_strategy{1.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        EXPECT_NEAR( calc(ctx), -sb, 1e-5);
}

TEST_F(hu_solver, push_fold){
        calc_context ctx = {
                &cec,
                hu_strategy{1.0},
                hu_strategy{0.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        EXPECT_NEAR( calc(ctx), bb, 1e-5);
}
TEST_F(hu_solver, push_call){
        calc_context ctx = {
                &cec,
                hu_strategy{1.0},
                hu_strategy{1.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        EXPECT_NEAR( calc(ctx), 0.0, 1e-5);
}

TEST_F(hu_solver, push_half_call){
        calc_context ctx = {
                &cec,
                hu_strategy{1.0},
                hu_strategy{0.5},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        EXPECT_NEAR( calc(ctx), .5 * bb, 1e-5);
}
