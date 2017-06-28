#include <gtest/gtest.h>

TEST(suit_decl, _){
        for(suit_id id{0};id!=4;++id){
                EXPECT_EQ(id, suit_decl::get(id).id());
        }
        EXPECT_NO_THROW( suit_decl::parse("d") );
        EXPECT_NO_THROW( suit_decl::parse("c") );
        EXPECT_NO_THROW( suit_decl::parse("h") );
        EXPECT_NO_THROW( suit_decl::parse("s") );
        EXPECT_NO_THROW( suit_decl::parse("D") );
        EXPECT_NO_THROW( suit_decl::parse("C") );
        EXPECT_NO_THROW( suit_decl::parse("H") );
        EXPECT_NO_THROW( suit_decl::parse("S") );
        EXPECT_NO_THROW( suit_decl::parse('d') );
        EXPECT_NO_THROW( suit_decl::parse('c') );
        EXPECT_NO_THROW( suit_decl::parse('h') );
        EXPECT_NO_THROW( suit_decl::parse('s') );
        EXPECT_NO_THROW( suit_decl::parse('D') );
        EXPECT_NO_THROW( suit_decl::parse('C') );
        EXPECT_NO_THROW( suit_decl::parse('H') );
        EXPECT_NO_THROW( suit_decl::parse('S') );
}
TEST(rank_decl, _){
        for(rank_id id{0};id!=13;++id){
                EXPECT_EQ(id, rank_decl::get(id).id());
        }
        EXPECT_NO_THROW( suit_decl::parse("A") );
        EXPECT_NO_THROW( suit_decl::parse("K") );
        EXPECT_NO_THROW( suit_decl::parse("Q") );
        EXPECT_NO_THROW( suit_decl::parse("J") );
        EXPECT_NO_THROW( suit_decl::parse("T") );
        EXPECT_NO_THROW( suit_decl::parse("a") );
        EXPECT_NO_THROW( suit_decl::parse("k") );
        EXPECT_NO_THROW( suit_decl::parse("q") );
        EXPECT_NO_THROW( suit_decl::parse("j") );
        EXPECT_NO_THROW( suit_decl::parse("t") );
        EXPECT_NO_THROW( suit_decl::parse("9") );
        EXPECT_NO_THROW( suit_decl::parse("8") );
        EXPECT_NO_THROW( suit_decl::parse("7") );
        EXPECT_NO_THROW( suit_decl::parse('6') );
        EXPECT_NO_THROW( suit_decl::parse('5') );
        EXPECT_NO_THROW( suit_decl::parse('4') );
        EXPECT_NO_THROW( suit_decl::parse('3') );
        EXPECT_NO_THROW( suit_decl::parse('2') );
}
TEST(card_decl, _){
        for(card_id id{0};id!=52;++id){
                EXPECT_EQ(id, card_decl::get(id).id());
        }
}
TEST(card_decl, _){

        // first card is A
        EXPECT_EQ( card::decl::get(0).rank(), rank_decl::parse("A") );

        EXPECT_EQ( card_decl::parse("Ah").suit(), suit_decl::parse("h") );
        EXPECT_EQ( card_decl::parse("Ah").rank(), rank_decl::parse("A") );
}
