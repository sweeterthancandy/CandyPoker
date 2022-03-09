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

#include "ps/base/cards.h"
#include "ps/base/algorithm.h"
#include "ps/support/float.h"

using namespace ps;

TEST(suit_decl, _){
        for(suit_id id{0};id!=4;++id){
                EXPECT_EQ(id, suit_decl::get(id).id());
        }
        EXPECT_EQ( decl::_d, suit_decl::parse("d") );
        EXPECT_EQ( decl::_d, suit_decl::parse('D') );
        EXPECT_EQ( decl::_c, suit_decl::parse("c") );
        EXPECT_EQ( decl::_c, suit_decl::parse("C") );
        EXPECT_EQ( decl::_h, suit_decl::parse('h') );
        EXPECT_EQ( decl::_h, suit_decl::parse("H") );
        EXPECT_EQ( decl::_s, suit_decl::parse('s') );
        EXPECT_EQ( decl::_s, suit_decl::parse("S") );
}
TEST(rank_decl, _){
        for(rank_id id{0};id!=13;++id){
                EXPECT_EQ(id, rank_decl::get(id).id());
        }
        EXPECT_EQ( decl::_A, rank_decl::parse("A") );
        EXPECT_EQ( decl::_A, rank_decl::parse("a") );
        EXPECT_EQ( decl::_K, rank_decl::parse("K") );
        EXPECT_EQ( decl::_K, rank_decl::parse("k") );
        EXPECT_EQ( decl::_Q, rank_decl::parse("Q") );
        EXPECT_EQ( decl::_Q, rank_decl::parse("q") );
        EXPECT_EQ( decl::_J, rank_decl::parse("J") );
        EXPECT_EQ( decl::_J, rank_decl::parse("j") );
        EXPECT_EQ( decl::_T, rank_decl::parse("T") );
        EXPECT_EQ( decl::_T, rank_decl::parse("t") );
        EXPECT_EQ( decl::_9, rank_decl::parse("9") );
        EXPECT_EQ( decl::_8, rank_decl::parse("8") );
        EXPECT_EQ( decl::_7, rank_decl::parse("7") );
        EXPECT_EQ( decl::_6, rank_decl::parse("6") );
        EXPECT_EQ( decl::_5, rank_decl::parse("5") );
        EXPECT_EQ( decl::_4, rank_decl::parse("4") );
        EXPECT_EQ( decl::_3, rank_decl::parse("3") );
        EXPECT_EQ( decl::_2, rank_decl::parse("2") );


        EXPECT_EQ( 12, rank_decl::parse("A").id() );
        EXPECT_EQ( 0, rank_decl::parse("2").id() );
}
TEST(card_decl, _){
        for(card_id id{0};id!=52;++id){
                EXPECT_EQ(id, card_decl::get(id).id());
        }

        // first card is A
        EXPECT_EQ( card_decl::get(0).rank(), rank_decl::parse("2") );
        EXPECT_EQ( card_decl::get(51).rank(), rank_decl::parse("A") );

        EXPECT_EQ( card_decl::parse("Ah").suit(), suit_decl::parse("h") );
        EXPECT_EQ( card_decl::parse("Ah").rank(), rank_decl::parse("A") );
}


template<typename T>
static std::string binary_str_cast(const T& x)
{
    std::stringstream ss;
    ss << std::bitset<sizeof(T) * 8>(x);
    return ss.str();
}


TEST(card_vector,_)
{
        auto c0 = card_decl::parse("2h");
        auto c1 = card_decl::parse("Ts");

        card_vector cv{c0.id(),c1.id()};

        EXPECT_EQ( c0.mask() | c1.mask(), cv.mask());
        EXPECT_EQ( detail::popcount(c0.mask()), 1);
        EXPECT_EQ( detail::popcount(c1.mask()), 1);
        EXPECT_EQ( detail::popcount(cv.mask()), 2);
}

TEST(holdem_hand_decl, mask)
{
        auto c0 = card_decl::parse("As");
        auto c1 = card_decl::parse("Ks");
        
        auto c2 = card_decl::parse("Tc");
        auto c3 = card_decl::parse("Ts");

        holdem_hand_decl h0{c0,c1};
        holdem_hand_decl h1{c2,c3};
        
        holdem_hand_decl h2{c0,c3};

        EXPECT_EQ( c0.mask() | c1.mask(), holdem_hand_decl(c0,c1).mask());

        EXPECT_FALSE( disjoint(h0,h0));
        EXPECT_TRUE( disjoint(h0,h1));
        EXPECT_FALSE( disjoint(h0,h2));

}


TEST(holdem_hand_vector,parser)
{
        std::string c0_expr{"AhAc"};
        std::string c1_expr{"KhKc"};
        std::string c2_expr{"8s7s"};
        std::string hv_expr{c0_expr+c1_expr+c2_expr};


        const auto c0 = holdem_hand_decl::parse(c0_expr);
        const auto c1 = holdem_hand_decl::parse(c1_expr);
        const auto c2 = holdem_hand_decl::parse(c2_expr);

        const auto hv = holdem_hand_vector::parse(hv_expr);
        const holdem_hand_vector expected_hv({c0.id(), c1.id(), c2.id()});

        EXPECT_EQ( 3, hv.size());
        EXPECT_EQ( hv.to_set(), expected_hv.to_set());

}



TEST(holdem_class_decl, parse){
        holdem_class_decl hc = holdem_class_decl::parse("TT");
        holdem_hand_vector hv = hc.get_hand_vector();


        std::string expected_expr;
        suit_decl::visit_suit_combinations(
                [&expected_expr](suit_decl const& a, suit_decl const& b)
                {
                        expected_expr += std::string{"T"} + a.symbol() + "T" + b.symbol();
                });
        holdem_hand_vector expected_hv = holdem_hand_vector::parse(expected_expr);
        EXPECT_EQ( expected_hv.size(), 6);

        EXPECT_EQ( hv.to_set(), expected_hv.to_set());

}


TEST(holdem_class_decl, prob){
        double sigma{0.0};
        for(holdem_id i{0};i!=169;++i){
                sigma += holdem_class_decl::get(i).prob();
        }
        EXPECT_NEAR(1.0, sigma, 1e-3);
        EXPECT_NEAR( ( 4 / 52.0 ) * (3 / 52.0) , holdem_class_decl::parse("AA").prob(), 1e-3);
}

TEST(holdem_class_decl, static_prob){
        double sigma{0.0};
        for(holdem_id i{0};i!=169;++i){
                for(holdem_id j{0};j!=169;++j){
                        sigma += holdem_class_decl::prob(i,j);
                }
        }
        EXPECT_NEAR(1.0, sigma, 1e-3);
}
TEST(holdem_class_decl, class_){
        for(holdem_id i{0};i!=169;++i){
                auto const& decl = holdem_class_decl::get(i);
                for( auto c : decl.get_hand_set() ){
                        EXPECT_EQ(decl.id(), c.class_());
                }
        }
}

TEST(holdem_class_vector, to_standard_form)
{
        const auto cv = holdem_class_vector::parse("AA,AA,KK");
        std::cout << cv << "\n";
        const auto ret = cv.to_standard_form();

        std::cout << std_vector_to_string(std::get<0>(ret)) << "\n";
        std::cout << std::get<1>(ret) << "\n";
}

