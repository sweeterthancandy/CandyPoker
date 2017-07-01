#include <gtest/gtest.h>

#include "ps/cards.h"

using namespace ps;

TEST(suit_decl, _){
        for(suit_id id{0};id!=4;++id){
                EXPECT_EQ(id, suit_decl::get(id).id());
        }
        EXPECT_NO_THROW( suit_decl::parse("d") );
        EXPECT_NO_THROW( suit_decl::parse("c") );
        EXPECT_NO_THROW( suit_decl::parse('h') );
        EXPECT_NO_THROW( suit_decl::parse('s') );
        EXPECT_NO_THROW( suit_decl::parse('D') );
        EXPECT_NO_THROW( suit_decl::parse("C") );
        EXPECT_NO_THROW( suit_decl::parse("H") );
        EXPECT_NO_THROW( suit_decl::parse("S") );
}
TEST(rank_decl, _){
        for(rank_id id{0};id!=13;++id){
                EXPECT_EQ(id, rank_decl::get(id).id());
        }
        EXPECT_NO_THROW( rank_decl::parse("A") );
        EXPECT_NO_THROW( rank_decl::parse("K") );
        EXPECT_NO_THROW( rank_decl::parse("Q") );
        EXPECT_NO_THROW( rank_decl::parse("J") );
        EXPECT_NO_THROW( rank_decl::parse("T") );
        EXPECT_NO_THROW( rank_decl::parse("a") );
        EXPECT_NO_THROW( rank_decl::parse("k") );
        EXPECT_NO_THROW( rank_decl::parse("q") );
        EXPECT_NO_THROW( rank_decl::parse("j") );
        EXPECT_NO_THROW( rank_decl::parse("t") );
        EXPECT_NO_THROW( rank_decl::parse("9") );
        EXPECT_NO_THROW( rank_decl::parse("8") );
        EXPECT_NO_THROW( rank_decl::parse("7") );
        EXPECT_NO_THROW( rank_decl::parse('6') );
        EXPECT_NO_THROW( rank_decl::parse('5') );
        EXPECT_NO_THROW( rank_decl::parse('4') );
        EXPECT_NO_THROW( rank_decl::parse('3') );
        EXPECT_NO_THROW( rank_decl::parse('2') );


        EXPECT_EQ( 12, rank_decl::parse("A").id() );
        EXPECT_EQ( 0, rank_decl::parse("2").id() );
}
TEST(card_decl, _){
        for(card_id id{0};id!=52;++id){
                EXPECT_EQ(id, card_decl::get(id).id());
        }

        // first card is A
        EXPECT_EQ( card_decl::get(0).rank(), rank_decl::parse("2") );

        EXPECT_EQ( card_decl::parse("Ah").suit(), suit_decl::parse("h") );
        EXPECT_EQ( card_decl::parse("Ah").rank(), rank_decl::parse("A") );
}

TEST(holdem_class_decl, prob){
        double sigma{0.0};
        for(size_t i{0};i!=169;++i){
                sigma += holdem_class_decl::get(i).prob();
        }
        EXPECT_NEAR(1.0, sigma, 1e-3);
        EXPECT_NEAR( ( 4 / 52.0 ) * (3 / 52.0) , holdem_class_decl::parse("AA").prob(), 1e-3);
}
TEST(holdem_class_decl, static_prob){
        double sigma{0.0};
        for(size_t i{0};i!=169;++i){
                for(size_t j{0};j!=169;++j){
                        sigma += holdem_class_decl::prob(i,j);
                }
        }
        EXPECT_NEAR(1.0, sigma, 1e-3);
}
TEST(holdem_class_decl, class_){
        for(size_t i{0};i!=169;++i){
                auto const& decl{holdem_class_decl::get(i)};
                for( auto c : decl.get_hand_set() ){
                        EXPECT_EQ(decl.id(), c.class_());
                }
        }
}
