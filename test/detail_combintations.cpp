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
                [](auto a, auto b, auto c){
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
