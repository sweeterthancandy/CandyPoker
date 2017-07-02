
#include <gtest/gtest.h>
#include "ps/frontend.h"
#include "ps/tree.h"
#include "ps/detail/print.h"

using namespace ps;
using namespace ps::frontend;

TEST( frontend, parse){
        parse("ATo");
        parse("AKo");
        parse("ATo-AKo");
        parse("44");
        parse("44+");
        parse("44+,AQs+");
        parse("   22-55");
        parse("  ajo-kk ");
        parse("          22+ Ax+ K2s+ K6o+ Q4s+ Q9o+ J6s+ J9o+ T6s+ T8o+ 96s+ 98o 85s+ 75s+ 64s+ 54s");
}

TEST( frontend, to_class_id_){
        auto rng{ expand(percent(100)) };
        auto prim_rng{ rng.to_primitive_range() };
        EXPECT_EQ( prim_rng.size(), 169 );

        tree_range root{ std::vector<frontend::range>{rng, rng} };

        for( auto const& c : root.children ){
                for(size_t i=0;i!=c.players.size();++i){
                        EXPECT_NE(-1,to_class_id(c.players[i]));
                }
                for( auto d : c.children ){
                }
        }
}
