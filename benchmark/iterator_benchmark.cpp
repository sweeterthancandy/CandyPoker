#include <benchmark/benchmark.h>

#include <thread>
#include <numeric>
#include <atomic>
#include <boost/format.hpp>
#include "ps/support/config.h"
#include "ps/base/frontend.h"
#include "ps/base/cards.h"
#include "ps/base/tree.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/support/index_sequence.h"
#include "ps/eval/evaluator_5_card_map.h"
#include "ps/eval/evaluator_6_card_map.h"
#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/eval/instruction.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/eval/class_cache.h"
#include "ps/eval/pass.h"
#include "ps/eval/pass_eval_hand_instr_vec.h"
#include "ps/base/rank_board_combination_iterator.h"

#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>
#include <fstream>

#include "ps/support/command.h"
#include "ps/support/persistent.h"
#include "ps/eval/holdem_class_vector_cache.h"

using namespace ps;

#if 0
 std::cout << std::fixed;
                n = 0;
                std::cout << "rank_board_combination_iterator\n";
                for(rank_board_combination_iterator iter(7),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }
                return 0;


                std::cout << "holdem_hand_iterator\n";
                n = 0;
                for(holdem_hand_iterator iter(5),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }

                std::cout << "holdem_class_iterator\n";
                n = 0;
                for(holdem_class_iterator iter(3),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }

                std::cout << "board_combination_iterator\n";
                n = 0;
                for(board_combination_iterator iter(3),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }
                
                std::cout << "holdem_class_perm_iterator<2>\n";
                n = 0;
                for(holdem_class_perm_iterator iter(2),end;iter!=end && n < MaxIter;++iter,++n){
                        auto p = iter->prob();
                        std::cout << "    " <<  *iter << " - " << p << "\n";
                }
                
                std::cout << "holdem_class_perm_iterator<3>\n";
                n = 0;
                for(holdem_class_perm_iterator iter(3),end;iter!=end && n < MaxIter;++iter,++n){
                        auto p = iter->prob();
                        std::cout << "    " <<  *iter << " - " << p << "\n";
                }

                double sigma = 0.0;
                for(holdem_class_perm_iterator iter(2),end;iter!=end;++iter){
                        sigma += iter->prob();
                }
                std::cout << "sigma<2> => " << sigma << "\n"; // __CandyPrint__(cxx-print-scalar,sigma)



#endif

static void Iterator_BitmaskFiveCardBoard(benchmark::State& state) {

        //constexpr size_t end_board = 0b1111100000000000000000000000000000000000000000000000ull;
        constexpr size_t end_board = 0b11111000000000000000000000000000ull;
        constexpr size_t end = end_board + 1;
        for (auto _ : state)
        { 
                size_t counter = 0;      
                for (size_t iter = 0; iter < end; ++iter)
                {
                        if (detail::popcount(iter) != 5)
                        {
                                continue;
                        }
                        ++counter;

                        benchmark::DoNotOptimize(iter);
                }
                std::cout << "counter = " << counter << "\n";

        }    
}
static void Iterator_FiveCardBoard(benchmark::State& state) {

        for (auto _ : state)
        {  
                for(board_combination_iterator iter(5),end;iter!=end ;++iter){
                        benchmark::DoNotOptimize(*iter);
                }
        }    
}
BENCHMARK(Iterator_FiveCardBoard)->Unit(benchmark::kSecond);


static void Iterator_ThreePlayerClass(benchmark::State& state) {

        for (auto _ : state)
        {  
                for(holdem_class_iterator iter(3),end;iter!=end;++iter){
                        benchmark::DoNotOptimize(*iter);
                }
        }    
}
BENCHMARK(Iterator_ThreePlayerClass)->Unit(benchmark::kSecond);


BENCHMARK_MAIN();