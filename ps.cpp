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

        size_t count=0;
        size_t other=0;
        for( holdem_hand_deal_iterator iter(2),end;iter!=end;++iter){
                #if 0
                std::stringstream sstr;
                sstr << *iter << " " << iter->disjoint() << "\n";
                std::cout << sstr.str();
                #endif
                ++count;
        }

        for(size_t a=0;a!=51;++a){
                for(size_t b=a+1;b!=52;++b){
                        for(size_t c=0;c!=51;++c){
                                for(size_t d=c+1;d!=52;++d){
                                        auto _1 = 1ull;
                                        decltype( _1) mask = 0ull;
                                        mask |= _1 << a;
                                        mask |= _1 << b;
                                        if( mask & ( _1 << c )  )
                                                continue;
                                        if( mask & ( _1 << d )  )
                                                continue;
                                        ++other;
                                        holdem_hand_vector aux;
                                        aux.push_back( holdem_hand_decl::make_id(a,b));
                                        aux.push_back( holdem_hand_decl::make_id(c,d));
                               }
                        }
               }
        }
        PRINT_SEQ((count)(other));
}


