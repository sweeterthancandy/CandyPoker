#include <gtest/gtest.h>

#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

using namespace ps;

TEST( combintations, _){
        std::cout << "visit_combinations\n";
        detail::visit_combinations<3>(
                [](auto a, auto b, auto c){
                PRINT_SEQ((a)(b)(c));
        }, detail::true_, 4);

        std::cout << "\n visit_exclusive_combinations \n";
        detail::visit_exclusive_combinations<3>(
                [](auto a, auto b, auto c){
                PRINT_SEQ((a)(b)(c));
        }, detail::true_, std::vector<size_t>{1,3,2});

        std::cout << "\n visit_exclusive_combinations_vf \n";
        detail::visit_exclusive_combinations_vf<3>(
                [](auto a, auto b, auto c){
                PRINT_SEQ((a)(b)(c));
        }, detail::true_, std::vector<size_t>{1,3,2});

}
