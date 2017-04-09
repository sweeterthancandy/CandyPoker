#include <gtest/gtest.h>

#include "ps/ps.h"

using namespace ps;

TEST(card_traits_, _){
        card_traits t;
        const char* cards [] = {
                "As", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "Ts", "js", "qs", "Ks",
                "AD", "2D", "3D", "4D", "5D", "6D", "7D", "8D", "9D", "TD", "jD", "qD", "KD",
                "Ac", "2c", "3c", "4c", "5c", "6c", "7c", "8c", "9c", "Tc", "jc", "qc", "Kc",
                "Ah", "2h", "3h", "4h", "5h", "6h", "7h", "8h", "9h", "Th", "jh", "qh", "Kh" };
        std::set<long> s;
        for(long i=0;i!=sizeof(cards)/sizeof(void*);++i){
                long c = t.make(cards[i]);
                s.insert(c);
        }
        EXPECT_EQ(52,s.size());

        EXPECT_EQ( t.map_suit('c'), t.suit(t.make("Ac")));
        EXPECT_EQ( t.map_rank('A'), t.rank(t.make("Ac")));
        
        EXPECT_EQ( t.map_suit('d'), t.suit(t.make("6D")));
        EXPECT_EQ( t.map_rank('6'), t.rank(t.make("6D")));
}
