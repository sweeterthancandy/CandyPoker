#include "ps/holdem/simulation.h"
#include "ps/core/cards.h"

using namespace ps;

int main(){
        /*
                +----+------+-----------+---------+
                |Hand|Equity|   Wins    |  Ties   |
                +----+------+-----------+---------+
                | 55 |29.36%|115,460,928|1,331,496|
                |ako |38.25%|150,538,824|1,331,496|
                |89s |32.40%|127,445,904|1,331,496|
                +----+------+-----------+---------+

                +----+------+----------+-------+
                |Hand|Equity|   Wins   | Ties  |
                +----+------+----------+-------+
                |Ako |45.38%|55,669,464|562,284|
                | 55 |54.62%|67,054,140|562,284|
                +----+------+----------+-------+
        */

        ps::simulation_calc sc;
        simulation_context_maker maker;

        maker.begin_player();
        maker.add("55");
        maker.end_player();
        

        maker.begin_player();
        maker.add("AKo");
        maker.end_player();
        
        #if 1
        maker.begin_player();
        maker.add("89s");
        maker.end_player();
        #endif

        maker.debug();

        auto ctx{maker.compile()};


        sc.run(ctx);
        
}
