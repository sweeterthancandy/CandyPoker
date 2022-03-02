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
using namespace ps::frontend;


template <class ...Args>
static void HoldemHandVector_Permutate(benchmark::State& state, Args&&... args) {

        const auto hv = holdem_hand_vector::parse(args...);
                
        for (auto _ : state)
        {  
                auto p =  permutate_for_the_better(hv) ;
                benchmark::DoNotOptimize(p);
        }    
}
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAsKd, std::string("JcTc7s7dAsKd"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAdQsAsKd, std::string("JcTc7s7dAdQsAsKd"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAdQsAsKd8d9d, std::string("JcTc7s7dAdQsAsKd8d9d"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAdQsAsKd8d9d3c2s, std::string("JcTc7s7dAdQsAsKd8d9d3c2s"))->Unit(benchmark::kMillisecond);




template <class ...Args>
void ClassVector_GetHandVectors(benchmark::State& state, Args&&... args) {
       
        const auto cv = holdem_class_vector::parse(args...);
                
        for (auto _ : state)
        {  
                for( auto hv : cv.get_hand_vectors()){
                        benchmark::DoNotOptimize(hv);
                }
        }    
}
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK, std::string("AA,KK"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ, std::string("AA,KK,QQ"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, QQ_KK_AA, std::string("QQ,KK,AA"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ_JJ, std::string("AA,KK,QQ,JJ"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ_JJ_TT, std::string("AA,KK,QQ,JJ,TT"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo, std::string("AKo,QJo"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o, std::string("AKo,QJo,T9o"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o_87o, std::string("AKo,QJo,T9o,87o"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o_87o_65o, std::string("AKo,QJo,T9o,87o,65o"))->Unit(benchmark::kMillisecond);

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
BENCHMARK(Iterator_FiveCardBoard)->Unit(benchmark::kMillisecond);


static void Iterator_ThreePlayerClass(benchmark::State& state) {

        for (auto _ : state)
        {  
                for(holdem_class_iterator iter(3),end;iter!=end;++iter){
                        benchmark::DoNotOptimize(*iter);
                }
        }    
}
BENCHMARK(Iterator_ThreePlayerClass)->Unit(benchmark::kMillisecond);



static void Vs_ClassRange_First1000(benchmark::State& state, size_t n) {

        for (auto _ : state)
        {  
                size_t counter = 0;
                for(holdem_class_iterator iter(n),end;iter!=end;++iter,++counter){
                        auto const& cv = *iter;
                        for( auto hv : cv.get_hand_vectors()){
                                benchmark::DoNotOptimize(hv);
                        }
                        if( counter == 1000 )
                                break;
                }
        }    
}
BENCHMARK_CAPTURE(Vs_ClassRange_First1000, Two, 2)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(Vs_ClassRange_First1000, Three, 3)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(Vs_ClassRange_First1000, Four, 4)->Unit(benchmark::kMillisecond);


BENCHMARK_MAIN();
