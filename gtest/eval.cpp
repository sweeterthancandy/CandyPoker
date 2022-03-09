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

TEST(Eval, KK_vs_AA) {
        std::vector<std::string> player_ranges{  "KK", "AA" };
        auto result = evaluate(player_ranges);

        const auto P0 = result.player_view(1);
        const auto P1 = result.player_view(0);

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

TEST(Eval, MultipleSims)
{
        std::vector<std::string> first_eval_ranges{ "AA", "KK" };
        std::vector<std::string> second_eval_ranges{ "AKs,AQs,AKo", "AA,KK,QQ" };

        auto result_list = evaluate_list({first_eval_ranges, second_eval_ranges});
        ASSERT_EQ(result_list.size(),2);

        {
                auto result = result_list[0];
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
        {
                auto result = result_list[1];
                const auto P0 = result.player_view(0);
                const auto P1 = result.player_view(1);

                EXPECT_EQ( P0.Wins(), 125'645'556 );
                EXPECT_EQ( P0.AnyDraws(), 3'148'680 );
                
                EXPECT_EQ( P1.Wins(), 282'158'724 );
                EXPECT_EQ( P1.AnyDraws(), 3'148'680 );

                EXPECT_EQ( P0.EquityAsRational(), rational_ty(588981,1902560));
                EXPECT_EQ( P1.EquityAsRational(), rational_ty(1313579,1902560));
        }
}

TEST(Eval, ThreePlayerCardPerm)
{
        struct perm_ty
        {
                std::vector<std::string> player_ranges;
                size_t AA0_index;
                size_t AA1_index;
                size_t KK0_index;
        };
        std::vector<perm_ty> perm_list = {
                perm_ty{ { "AhAd", "AcAs", "KsKd" }, 0, 1, 2 },
                perm_ty{ { "AhAd", "KsKd", "AcAs" }, 0, 2, 1 },
                perm_ty{ { "KsKd", "AhAd", "AcAs" }, 1, 2, 0 }
        };
        for (auto const& perm : perm_list)
        {
                auto result = evaluate(perm.player_ranges);

                const auto AA0 = result.player_view(perm.AA0_index);
                const auto AA1 = result.player_view(perm.AA1_index);
                const auto KK0 = result.player_view(perm.KK0_index);

                EXPECT_EQ( AA0.Wins(), 	27'939 ) << std_vector_to_string(perm.player_ranges);
                EXPECT_EQ( AA0.Wins(), 	AA1.Wins() ) << std_vector_to_string(perm.player_ranges);
                EXPECT_EQ( AA0.AnyDraws(), AA1.AnyDraws() ) << std_vector_to_string(perm.player_ranges);

                EXPECT_EQ( KK0.Wins(), 278'116  ) << std_vector_to_string(perm.player_ranges);
                EXPECT_EQ( KK0.AnyDraws(), 5'786 ) << std_vector_to_string(perm.player_ranges);
        }
       
        
}

TEST(Eval, ThreePlayerClassPerm)
{
        struct perm_ty
        {
                std::vector<std::string> player_ranges;
                size_t AA0_index;
                size_t AA1_index;
                size_t KK0_index;
        };
        std::vector<perm_ty> perm_list = {
                perm_ty{ { "AA", "AA", "KK" }, 0, 1, 2 },
                perm_ty{ { "AA", "KK", "AA" }, 0, 2, 1 },
                perm_ty{ { "KK", "AA", "AA" }, 1, 2, 0 }
        };
        for (auto const& perm : perm_list)
        {
                auto result = evaluate(perm.player_ranges);

                const auto AA0 = result.player_view(perm.AA0_index);
                const auto AA1 = result.player_view(perm.AA1_index);
                const auto KK0 = result.player_view(perm.KK0_index);

                EXPECT_EQ( AA0.Wins(), 	AA1.Wins() ) << std_vector_to_string(perm.player_ranges);
                EXPECT_EQ( AA0.AnyDraws(), AA1.AnyDraws() ) << std_vector_to_string(perm.player_ranges);

                EXPECT_EQ( KK0.Wins(), 10'012'176  ) << std_vector_to_string(perm.player_ranges);
                EXPECT_EQ( KK0.AnyDraws(), 208'296 ) << std_vector_to_string(perm.player_ranges);
        }
       
        
}