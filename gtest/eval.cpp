#include <gtest/gtest.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"

using namespace ps;
using namespace ps::interface_;

// Demonstrate some basic assertions.
TEST(Eval, AA_vs_KK) {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        auto result = evaluate(player_ranges);
        for(auto const& p : result.players())
        {
                std::cout << "p.EquityAsRational() => " << p.EquityAsRational() << "\n"; // __CandyPrint__(cxx-print-scalar,p.EquityAsRational())
                std::cout << "p.EquityAsDouble() => " << p.EquityAsDouble() << "\n"; // __CandyPrint__(cxx-print-scalar,p.EquityAsDouble())
        }

}


