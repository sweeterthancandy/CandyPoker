#include "ps/heads_up_solver.h"

#include <random>

int main(){
        using namespace ps;

        equity_cacher ec;
        ec.load("cache.bin");
        class_equity_cacher cec(ec);
        cec.load("hc_cache.bin");

        double eff_stack{10.0};
        double bb{1.0};
        double sb{0.5};

        hu_strategy{1.0}.display();
                        
        auto sb_strat{solve_hu_push_fold_sb(cec, eff_stack, sb, bb)};
        auto bb_strat{solve_hu_push_fold_bb_maximal_exploitable(cec,
                                                                sb_strat,
                                                                eff_stack,
                                                                sb,
                                                                bb)};
        sb_strat.transform( [](auto i, auto d){
                if( std::fabs( d - 0.0 ) < 1e-3 )
                        return 0.0;
                if( std::fabs( d - 1.0 ) < 1e-3 )
                        return 1.0;
                return d;
        });

        sb_strat.display();
        std::default_random_engine gen;
        std::uniform_int_distribution<holdem_class_id>(0,51);
                

}
