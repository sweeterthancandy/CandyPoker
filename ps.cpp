#include "ps/holdem/simulation.h"
#include "ps/core/cards.h"
#include "ps/holdem/frontend.h"

using namespace ps;
int main(){

        using namespace ps::frontend;

        range rng;
        rng += suited(0,1);
        rng += _AA;
        rng += _TT++;
        rng += _AQo-_ATo;

        PRINT(rng);

        PRINT( parse("ATo") );
        PRINT( parse("AKo") );
        PRINT( parse("ATo-AKo") );
        PRINT( parse("44") );
        PRINT( parse("44+") );
        PRINT( parse("44+,AQs+"));
        PRINT( parse("   22-55") );
        PRINT( parse("  ajo-kk ") );
        PRINT( parse("          22+ Ax+ K2s+ K6o+ Q4s+ Q9o+ J6s+ J9o+ T6s+ T8o+ 96s+ 98o 85s+ 75s+ 64s+ 54s") );
        
}
