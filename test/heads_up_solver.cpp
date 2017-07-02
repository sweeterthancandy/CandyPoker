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
