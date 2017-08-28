#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <bitset>
#include <cstdint>
#include <future>
#include <boost/format.hpp>
#include <boost/timer/timer.hpp>

#include "ps/support/config.h"
#include "ps/support/index_sequence.h"
#include "ps/base/cards.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/sim/holdem_class_strategy.h"

#include <boost/range/algorithm.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>

#include <random>



using namespace ps;

                

int main(){
        size_t sigma = 0;
        double sum = .0;
        for(holdem_class_deal_iterator iter(3),end;iter!=end;++iter){
                sigma += iter->weight();
                sum += iter->prob();
        }
        PRINT(sigma);
        PRINT(sum);
}


