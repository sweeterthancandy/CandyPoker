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
        const auto parsed_range = frontend::parse(expr);

            std::cout << "parsed_range => " << parsed_range << "\n"; // __CandyPrint__(cxx-print-scalar,parsed_range)
        holdem_hand_vector hv = parsed_range.to_holdem_vector();
        holdem_hand_vector expected_hv{
                holdem_hand_decl::parse(expr).id(),
        };

        EXPECT_EQ( hv.to_set(), expected_hv.to_set());
}

TEST(frontend, parser_holdem_class)
{
        const std::string expr{"TT"};
        const auto parsed_range = frontend::parse(expr);
    std::cout << "parsed_range => " << parsed_range << "\n"; // __CandyPrint__(cxx-print-scalar,parsed_range)
        holdem_hand_vector hv = parsed_range.to_holdem_vector();
        holdem_hand_vector expected_hv = holdem_class_decl::parse("TT").get_hand_vector();

        EXPECT_EQ( hv.to_set(), expected_hv.to_set());
}

TEST(frontend, parser_holdem_class_comma)
{
    const std::string class_expr0 = "TT";
    const std::string class_expr1 = "JTs";
    const std::string range_expr = class_expr0 + "," + class_expr1;
    
    const auto parsed_range = frontend::parse(range_expr);
    std::cout << "parsed_range => " << parsed_range << "\n"; // __CandyPrint__(cxx-print-scalar,parsed_range)
    holdem_hand_vector hv = parsed_range.to_holdem_vector();

    holdem_hand_vector expected_hv = holdem_hand_vector::union_(
        holdem_class_decl::parse(class_expr0).get_hand_vector(),
        holdem_class_decl::parse(class_expr1).get_hand_vector());

    EXPECT_EQ(hv.to_set(), expected_hv.to_set());
}

TEST(frontend, parser_holdem_class_plus_pp)
{
    const auto parsed_range = frontend::parse("TT+");
    std::cout << "parsed_range => " << parsed_range << "\n"; // __CandyPrint__(cxx-print-scalar,parsed_range)
    holdem_hand_vector hv = expand(parsed_range).to_holdem_vector();

    holdem_hand_vector expected_hv = holdem_hand_vector::union_(
        holdem_class_decl::parse("AA").get_hand_vector(),
        holdem_class_decl::parse("KK").get_hand_vector(),
        holdem_class_decl::parse("QQ").get_hand_vector(),
        holdem_class_decl::parse("JJ").get_hand_vector(),
        holdem_class_decl::parse("TT").get_hand_vector());

    EXPECT_EQ(hv.to_set(), expected_hv.to_set());
}

TEST(frontend, parser_holdem_class_plus_suited)
{
    const auto parsed_range = frontend::parse("T9s+");
    const auto expanded_range = expand(parsed_range);
    std::cout << "parsed_range => " << parsed_range << "\n"; // __CandyPrint__(cxx-print-scalar,parsed_range)
    std::cout << "expanded_range => " << expanded_range << "\n"; // __CandyPrint__(cxx-print-scalar,expanded_range)
    holdem_hand_vector hv = expand(parsed_range).to_holdem_vector();

    holdem_hand_vector expected_hv = holdem_hand_vector::union_(
        holdem_class_decl::parse("AKs").get_hand_vector(),
        holdem_class_decl::parse("KQs").get_hand_vector(),
        holdem_class_decl::parse("QJs").get_hand_vector(),
        holdem_class_decl::parse("JTs").get_hand_vector(),
        holdem_class_decl::parse("T9s").get_hand_vector());

    EXPECT_EQ(hv.to_set(), expected_hv.to_set());
}
