#include <gtest/gtest.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"

using namespace ps;
using namespace ps::interface_;

/*

The integration tests are created by just finding examples which i assume to be true
 */

// Demonstrate some basic assertions.
TEST(Eval, AA_vs_KK) {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        auto result = evaluate(player_ranges);

        const auto P0 = result.player_view(0);
        const auto P1 = result.player_view(1);

        EXPECT_EQ( P0.Wins(), 50'371'344 );
        EXPECT_EQ( P0.AnyDraws(), 285'228 );
        EXPECT_EQ( P1.Wins(), 10'986'372 );
        EXPECT_EQ( P1.AnyDraws(), 285'228 );
        
        EXPECT_EQ( P0.EquityAsRational(), rational_ty(255121, 311328));
        EXPECT_EQ( P1.EquityAsRational(), rational_ty(56207, 311328));

        EXPECT_NEAR(P0.EquityAsDouble(), 0.8195, 0.0005);
        EXPECT_NEAR(P1.EquityAsDouble(), 0.1805, 0.0005);
}


