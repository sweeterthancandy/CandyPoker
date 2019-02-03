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
#include "ps/base/frontend.h"
#include "ps/base/tree.h"
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
        parse("100%");
}

TEST( frontend, to_class_id_){
        auto rng =  expand(percent(100)) ;
        auto prim_rng =  rng.to_primitive_range() ;
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
