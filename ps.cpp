#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/detail/print.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/sim/holdem_class_strategy.h"


/*
                Strategy in the form 
                        vpip/fold
 */
#include <boost/range/algorithm.hpp>

/*
        Need to,

                iterate over boards
                        abcde  where a < b < c < d < e ( think strict lower triangle )
                iterator over hands
                        ab  where a <= b               ( think lower triable )


 */



using namespace ps;



int main(){
        //for( holdem_class_iterator iter(2),end;iter!=end;++iter){
                //PRINT( *iter );
        //}
        //for( holdem_hand_iterator iter(2),end;iter!=end;++iter){
                //PRINT( *iter );
        //}
}
