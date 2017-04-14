#ifndef PS_HU_EQUITY_CALCULATOR_H
#define PS_HU_EQUITY_CALCULATOR_H

#include "ps/core/eval.h"

namespace ps{

        void hu_solver_test(){
                #if 0
                std::vector<long> push_strat( 52 * 51 / 2 , 1);  // push everything
                std::vector<long> fold_strat( 52 * 51 / 2 , 1);  // call everything
                #endif
                #if 0
                hu_equity_calc ec;
                PRINT( ec("AhKh", "js9c") );
                #endif
        }

} // namespace ps

#endif // PS_HU_EQUITY_CALCULATOR_H
