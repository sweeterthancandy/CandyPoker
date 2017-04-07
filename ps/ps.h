
// royal flush
// straight flush
// quad
// full house
// flush
// straight
// trips
// double pair
// pair
// high card

#pragma warning( disable:4530 )


#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>
#include <string>
#include <sstream>

#include <boost/array.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>



#include "card_traits.h"
#include "driver.h"
#include "eval.h"
#include "generate.h"
#include "printer.h"






#if 0
void traits_test(){
        const char* cards [] = {
                "As", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "Ts", "js", "qs", "Ks",
                "AD", "2D", "3D", "4D", "5D", "6D", "7D", "8D", "9D", "TD", "jD", "qD", "KD",
                "Ac", "2c", "3c", "4c", "5c", "6c", "7c", "8c", "9c", "Tc", "jc", "qc", "Kc",
                "Ah", "2h", "3h", "4h", "5h", "6h", "7h", "8h", "9h", "Th", "jh", "qh", "Kh" };

        card_traits t;
        for(long i=0;i!=sizeof(cards)/sizeof(void*);++i){
                long c = t.make(cards[i]);
                std::cout << cards[i] << " -> " << c << " " << t.rank(c) << " - " << t.suit(c) << "\n";
        }
        PRINT( t.make("Ac") );
        PRINT( t.make("5s") );
        PRINT( t.rank(t.make("Ah")) );
        PRINT( t.rank(t.make("Ac")) );
        PRINT( t.suit(t.make("Ah")) );
        PRINT( t.suit(t.make("Ac")) );
}

void eval_test(){
        driver d;

        PRINT( d.eval_5("AhKhQhJhTh") );
        PRINT( d.eval_5("AsKsQsJsTs") );
        PRINT( d.eval_5("AcKcQcJcTc") );
        PRINT( d.eval_5("AhAcAcAd2c") );
        PRINT( d.eval_5("AhAcAcAd2d") );

        std::cout << d.calc("AcKs", "2s2d");
}


void generate_test(){
        printer p;
        generate<card_traits>(p);

}
#endif
