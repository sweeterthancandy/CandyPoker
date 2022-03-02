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
static void HoldemHandVector_Permuate_3(benchmark::State& state) {

        const auto hv = holdem_hand_vector::parse("JcTc7s7dAsKd");
                
        for (auto _ : state)
        {  
                auto p =  permutate_for_the_better(hv) ;
                benchmark::DoNotOptimize(p);
        }    
}
BENCHMARK(HoldemHandVector_Permuate_3)->Unit(benchmark::kMillisecond);

static void HoldemHandVector_Permuate_4(benchmark::State& state) {

        const auto hv = holdem_hand_vector::parse("JcTc7s7dAdQsAsKd");
                
        for (auto _ : state)
        {  
                auto p =  permutate_for_the_better(hv) ;
                benchmark::DoNotOptimize(p);
        }    
}
BENCHMARK(HoldemHandVector_Permuate_4)->Unit(benchmark::kMillisecond);

static void HoldemHandVector_Permuate_5(benchmark::State& state) {

        const auto hv = holdem_hand_vector::parse("JcTc7s7dAdQsAsKd8d9d");
                
        for (auto _ : state)
        {  
                auto p =  permutate_for_the_better(hv) ;
                benchmark::DoNotOptimize(p);
        }    
}
BENCHMARK(HoldemHandVector_Permuate_5)->Unit(benchmark::kMillisecond);

                
static void ClassVector_Permutate_3(benchmark::State& state) {

        const auto cv = holdem_class_vector::parse("AA,KK,QQ");
                
        for (auto _ : state)
        {  
                for( auto hv : cv.get_hand_vectors()){
                        benchmark::DoNotOptimize(hv);
                }
        }    
}

BENCHMARK(ClassVector_Permutate_3)->Unit(benchmark::kMillisecond);



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
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ, std::string("AA,KK,QQ"));
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, QQ_KK_AA, std::string("QQ,KK,AA"));
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ_JJ, std::string("AA,KK,QQ,JJ"));
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ_JJ_TT, std::string("AA,KK,QQ,JJ,TT"));

BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo, std::string("AKo,QJo"));
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o, std::string("AKo,QJo,T9o"));
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o_87o, std::string("AKo,QJo,T9o,87o"));
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o_87o_65o, std::string("AKo,QJo,T9o,87o,65o"));

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


BENCHMARK_MAIN();
