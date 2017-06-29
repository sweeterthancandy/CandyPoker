#include "ps/heads_up.h"
#include "ps/detail/print.h"

#include <numeric>
#include <boost/timer/timer.hpp>

using namespace ps;
using namespace ps::frontend;

/*
        This is the solve the calling range in the sb given that the bb
        is showing with strat 
                hero_strat
 */
auto solve_hu_push_fold_bb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               std::vector<double> const& villian_strat,
                                               double eff_stack, double bb, double sb)
{
        std::vector<double> counter_strat(169, 0.0);
        for(holdem_class_id x{0}; x != 169;++x){
                /*
                 *  We call iff
                 *
                 *    EV<Call x| villian push> >= 0
                 *  =>
                 *    
                 *
                 */

                double win{0};
                double lose{0};
                double draw{0};
                // weight average again villians range
                for(holdem_class_id y{0}; y != 169;++y){
                        auto p{villian_strat[y]};
                        if( p == 0.0 )
                                continue;

                        auto const& sim{cec.visit_boards(std::vector<ps::holdem_class_id>{ x,y })};
                        win  += sim.win  * p;
                        lose += sim.lose * p;
                        draw += sim.draw * p;
                }
                double sigma{ win + lose + draw };
                double equity{ (win + draw / 2.0 ) / sigma};

                /*
                        Ev<Call> = equity * sizeofpot - costofcall
                        EV<Fold> = 0
                */
                double ev_call{ equity * 2 * ( eff_stack + bb ) - eff_stack };
                //double ev_fold{ 0.0 };
                auto hero{ holdem_class_decl::get(x) };
                //PRINT_SEQ((hero)(sigma)(equity)(ev_call));

                if( ev_call >= 0 ){
                        counter_strat[x] = 1.0;
                }

        }
        PRINT_SEQ(( std::accumulate(counter_strat.begin(), counter_strat.end(), 0.0) / 169 ));
        return std::move(counter_strat);
} 
auto solve_hu_push_fold_sb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               std::vector<double> const& villian_strat,
                                               double eff_stack, double bb, double sb)
{
        std::vector<double> counter_strat(169, 0.0);
        for(holdem_class_id x{0}; x != 169;++x){
                /*
                 *  We call iff
                 *
                 *    EV<push x| villian call with S> >= 0
                 */



                double win{0};
                double lose{0};
                double draw{0};

                double win_preflop{0};
                
                double sigma{0};

                // weight average again villians range
                for(holdem_class_id y{0}; y != 169;++y){
                        auto p{villian_strat[y]};

                        auto const& sim{cec.visit_boards(std::vector<ps::holdem_class_id>{ x,y })};
                        win  += sim.win  * p;
                        lose += sim.lose * p;
                        draw += sim.draw * p;

                        auto total{ sim.win + sim.lose + sim.draw };
                        sigma += total;
                        win_preflop += total * ( 1- p );

                }
                double call_sigma{ win + lose + draw };
                double call_equity{ (win + draw / 2.0 ) / call_sigma};
                
                double call_p{call_sigma / sigma };
                double fold_p{win_preflop / sigma};

                /*
                        Ev<Push> = Ev<Villan calls | Push> + Ev<Villian fold | Push>
                */
                double ev_push_call{ call_equity * 2 * ( eff_stack + sb ) - eff_stack };

                double ev_push{ call_p * ev_push_call + fold_p * ( sb + bb ) }; 
                
                auto hero{ holdem_class_decl::get(x) };

                //PRINT_SEQ((hero)(sigma)(call_equity)(call_p)(fold_p)(ev_push_call)(ev_push));

                if( ev_push >= 0 ){
                        counter_strat[x] = 1.0;
                }

        }
        PRINT_SEQ(( std::accumulate(counter_strat.begin(), counter_strat.end(), 0.0) / 169 ));
        return std::move(counter_strat);
} 


int main(){
        using namespace ps;
        using namespace ps::frontend;

        for(size_t i{0}; i != 13 * 13; ++ i){
                auto h{ holdem_class_decl::get(i) };
                PRINT_SEQ((i)(h)(ps::detail::to_string(h.get_hand_set())));
        } 
        equity_cacher ec;
        ec.load("cache.bin");
        class_equity_cacher cec(ec);
        cec.load("hc_cache.bin");

        #if 0
        auto _AKo{ holdem_class_decl::parse("AKo")};
        auto _33 { holdem_class_decl::parse("33" )};
        auto _JTs{ holdem_class_decl::parse("JTs")};

        PRINT( cec.visit_boards(
                        std::vector<ps::holdem_class_id>{
                                _AKo, _JTs } ) );
                                #endif
        double alpha{0.2};
        std::vector<double> sb_strat(169, 1.0);
        std::vector<double> bb_strat(169, 1.0);
        for(;;){

                std::vector<double> bb_me{solve_hu_push_fold_bb_maximal_exploitable(cec,
                                                                       sb_strat,
                                                                       10.0,
                                                                       1.0,
                                                                       0.5)};
                std::vector<double> sb_me{solve_hu_push_fold_sb_maximal_exploitable(cec,
                                                                       bb_strat,
                                                                       10.0,
                                                                       1.0,
                                                                       0.5)};
                #if 0
                PRINT_SEQ((::ps::detail::to_string(bb_me)));
                PRINT_SEQ((::ps::detail::to_string(sb_me)));
                #endif
                std::vector<double> sb_next(169,0.0);
                std::vector<double> bb_next(169,0.0);
                double sb_norm{0.0};
                double bb_norm{0.0};
                for(size_t i{0}; i != 13 * 13; ++ i){
                        sb_next[i] = sb_strat[i] * ( 1.0 - alpha ) + sb_me[i];
                        bb_next[i] = bb_strat[i] * ( 1.0 - alpha ) + bb_me[i];
                        sb_norm += std::pow(std::fabs( sb_next[i] - sb_strat[i] ), 2.0);
                        bb_norm += std::pow(std::fabs( bb_next[i] - bb_strat[i] ), 2.0);
                }
                sb_strat = std::move(sb_next);
                bb_strat = std::move(bb_next);
                PRINT_SEQ((sb_norm)(bb_norm));
                
        }
        #if 0
        for(size_t i{0};i!=3;++i){
                boost::timer::auto_cpu_timer at;
                for(holdem_class_id x{0}; x != 169;++x){
                        for(holdem_class_id y{0}; y != 169;++y){
                                auto hero{ holdem_class_decl::get(x) };
                                auto villian{ holdem_class_decl::get(y) };
                                cec.visit_boards( std::vector<ps::holdem_class_id>{ x,y } );
                                #if 0
                                PRINT_SEQ((hero)(villian)( cec.visit_boards(
                                                std::vector<ps::holdem_class_id>{
                                                        x,y } ) ));
                                                        #endif
                        }
                }
        }
        #endif

}
