
#include <gtest/gtest.h>
#include "ps/holdem/frontend.h"
#include "ps/detail/print.h"


TEST( frontend, _){

	using namespace ps;
        using namespace ps::frontend;

        range rng;
        rng += suited(0,1);
        rng += _AA;
        rng += _TT++;
        rng += _AQo-_ATo;

        PRINT(rng);

        parse("ATo");
        parse("AKo");
        parse("ATo-AKo");
        parse("44");
        parse("44+");
        parse("44+,AQs+");
        parse("   22-55");
        parse("  ajo-kk ");
        parse("          22+ Ax+ K2s+ K6o+ Q4s+ Q9o+ J6s+ J9o+ T6s+ T8o+ 96s+ 98o 85s+ 75s+ 64s+ 54s");
}
