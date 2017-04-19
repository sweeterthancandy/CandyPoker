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
        std::cout << "visit_combinations\n";
        detail::visit_combinations<3>(
                [](auto a, auto b, auto c){
                PRINT_SEQ((a)(b)(c));
        }, detail::true_, 4);
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
                PRINT_SEQ((a)(b)(c));
        }, detail::true_, std::vector<size_t>{1,3,2});
}
