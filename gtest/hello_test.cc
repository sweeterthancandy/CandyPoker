#include <gtest/gtest.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"

// Demonstrate some basic assertions.
TEST(Cards, AA) {
        auto range = ps::frontend::parse("AA");
        auto primitive_range = range.to_primitive_range();
        auto holdem_vector = range.to_holdem_vector();
        auto class_vec = range.to_class_vector();

        std::cout << "range => " << range << "\n"; // __CandyPrint__(cxx-print-scalar,range)
        std::cout << "primitive_range => " << primitive_range << "\n"; // __CandyPrint__(cxx-print-scalar,primitive_range)
        std::cout << "holdem_vector => " << holdem_vector << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_vector)
        std::cout << "class_vec => " << class_vec << "\n"; // __CandyPrint__(cxx-print-scalar,class_vec)
}


TEST(Cards, TreeRange) {
        auto sb_range = ps::frontend::parse("AJs+,AQo+");
        auto bb_range = ps::frontend::parse("QQ+");
        ps::tree_range root({sb_range, bb_range});


        std::cout << "root => " << root << "\n"; // __CandyPrint__(cxx-print-scalar,root)
        for(const auto& c : root.children )
        {
                std::cout << "c => " << c << "\n"; // __CandyPrint__(cxx-print-scalar,c)
                for(auto const& d : c.children )
                {
                        std::cout << "d => " << d << "\n"; // __CandyPrint__(cxx-print-scalar,d)
                }
        }
}
