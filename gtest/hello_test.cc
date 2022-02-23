#include <gtest/gtest.h>

#include "ps/base/frontend.h"


// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
        auto range = ps::frontend::parse("AA");
        auto primitive_range = range.to_primitive_range();
        auto holdem_vector = range.to_holdem_vector();
        auto class_vec = range.to_class_vector();

        std::cout << "range => " << range << "\n"; // __CandyPrint__(cxx-print-scalar,range)
        std::cout << "primitive_range => " << primitive_range << "\n"; // __CandyPrint__(cxx-print-scalar,primitive_range)
        std::cout << "holdem_vector => " << holdem_vector << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_vector)
        std::cout << "class_vec => " << class_vec << "\n"; // __CandyPrint__(cxx-print-scalar,class_vec)

}
