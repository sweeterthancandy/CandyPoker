#include "ps/holdem/simulation.h"
#include "ps/core/cards.h"

using namespace ps;

int main(){
        #if 0
        std::cout << "hello\n";

        simulation_context_maker maker;
        simulation_calc sc;

        maker.begin_player();
        maker.add("55");
        maker.end_player();

        maker.begin_player();
        maker.add("AK");
        maker.end_player();

        maker.debug();

        auto ctx{maker.compile()};


        sc.run(ctx);



        for( auto const& p : ctx.get_players()){
                std::cout << p << "\n";
        }
        #endif

        using namespace ps::decl;

        PRINT_SEQ((_2)(_A)(_Ah));
        PRINT_SEQ((suit_decl::get(0))(rank_decl::get(1)));
        PRINT_SEQ((card_decl::get(0)));
        PRINT_SEQ((card_decl::get(2)));
        PRINT_SEQ((card_decl::get(3)));
        PRINT_SEQ((card_decl::get(4)));



        //hu_solver_test();
}
