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
#include "ps/base/frontend.h"

using namespace ps;
using namespace ps::frontend;

TEST(frontend, parser_holdem_hand)
{
        const std::string expr{"TcTs"};
        const range = range::parse(expr);
        holdem_hand_vector hv = range.to_holdem_vector();
        holdem_hand_vector expected_hv{
                holdem_hand_decl::parse(expr).id(),
        };

        EXPECT_EQ( hv.to_set(), expected_hv.to_set());
}

TEST(frontend, parser_holdem_hand)
{
        const std::string expr{"TT"};
        const range = range::parse(expr);
        holdem_hand_vector hv = range.to_holdem_vector();
        holdem_hand_vector expected_hv{
                holdem_hand_decl::parse("TcTs").id(),
        };

        EXPECT_EQ( hv.to_set(), expected_hv.to_set());
}
