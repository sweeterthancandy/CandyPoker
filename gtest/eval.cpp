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


TEST(Eval, AAo_vs_22_vs_87s) {
        std::vector<std::string> player_ranges{ "AKo", "22", "87s" };
        auto result = evaluate(player_ranges);

        const auto P0 = result.player_view(0);
        const auto P1 = result.player_view(1);
        const auto P2 = result.player_view(2);

        EXPECT_EQ( P0.Wins(), 	150'166'584 );
        EXPECT_EQ( P0.AnyDraws(), 1'728'144 );

        EXPECT_EQ( P1.Wins(), 102'821'760  );
        EXPECT_EQ( P1.AnyDraws(), 1'728'144 );
        
        EXPECT_EQ( P2.Wins(), 140'060'664  );
        EXPECT_EQ( P2.AnyDraws(), 1'728'144 );
        
        EXPECT_EQ( P0.EquityAsRational(), rational_ty(6280943,16449048));
        EXPECT_EQ( P1.EquityAsRational(), rational_ty(2154121,8224524));
        EXPECT_EQ( P2.EquityAsRational(), rational_ty(5859863,16449048));

        EXPECT_NEAR(P0.EquityAsDouble(), 0.3818, 0.0005);
        EXPECT_NEAR(P1.EquityAsDouble(), 0.2619, 0.0005);
        EXPECT_NEAR(P2.EquityAsDouble(), 0.3562, 0.0005);
}



TEST(Eval, TwoPlayerRange) {
        std::vector<std::string> player_ranges{ "AKs,AQs,AKo", "AA,KK,QQ" };
        auto result = evaluate(player_ranges);

        const auto P0 = result.player_view(0);
        const auto P1 = result.player_view(1);

        EXPECT_EQ( P0.Wins(), 125'645'556 );
        EXPECT_EQ( P0.AnyDraws(), 3'148'680 );
        
        EXPECT_EQ( P1.Wins(), 282'158'724 );
        EXPECT_EQ( P1.AnyDraws(), 3'148'680 );

        EXPECT_EQ( P0.EquityAsRational(), rational_ty(588981,1902560));
        EXPECT_EQ( P1.EquityAsRational(), rational_ty(1313579,1902560));

}
