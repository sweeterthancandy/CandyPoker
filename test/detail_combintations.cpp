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

#include <tuple>

#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

using namespace ps;

TEST( visit_combinations, _){
        /*
        a = 4, b = 3, c = 2
        a = 4, b = 3, c = 1
        a = 4, b = 3, c = 0
        a = 4, b = 2, c = 1
        a = 4, b = 2, c = 0
        a = 4, b = 1, c = 0
        a = 3, b = 2, c = 1
        a = 3, b = 2, c = 0
        a = 3, b = 1, c = 0
        a = 2, b = 1, c = 0
        */
        std::vector<std::string> lines;
        detail::visit_combinations<3>(
                [&](auto a, auto b, auto c){
                std::stringstream sstr;
                sstr << a << b << c;
                lines.push_back(sstr.str());
                //PRINT_SEQ((a)(b)(c));
        }, detail::true_, 4);
        
        EXPECT_EQ( 10, lines.size() );
        EXPECT_EQ( "432", lines.front() );
        EXPECT_EQ( "210", lines.back() );
}

TEST( visit_exclusive_combinations, _){
        /*
        a = 1, b = 3, c = 2
        a = 0, b = 3, c = 2
        a = 1, b = 2, c = 2
        a = 0, b = 2, c = 2
        a = 1, b = 1, c = 2
        a = 0, b = 1, c = 2
        a = 1, b = 0, c = 2
        a = 0, b = 0, c = 2
        a = 1, b = 3, c = 1
        a = 0, b = 3, c = 1
        a = 1, b = 2, c = 1
        a = 0, b = 2, c = 1
        a = 1, b = 1, c = 1
        a = 0, b = 1, c = 1
        a = 1, b = 0, c = 1
        a = 0, b = 0, c = 1
        a = 1, b = 3, c = 0
        a = 0, b = 3, c = 0
        a = 1, b = 2, c = 0
        a = 0, b = 2, c = 0
        a = 1, b = 1, c = 0
        a = 0, b = 1, c = 0
        a = 1, b = 0, c = 0
        a = 0, b = 0, c = 0
        */
        detail::visit_exclusive_combinations<3>(
                [](auto a, auto b, auto c){
                //PRINT_SEQ((a)(b)(c));
        }, detail::true_, std::vector<size_t>{1,3,2});
}
