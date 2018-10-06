#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/eval/equity_breakdown.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/base/cards.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/holdem_class_eval_cache.h"
#include "pretty_printer.h"

#include <type_traits>
#include <functional>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/timer/timer.hpp>
#include <boost/log/trivial.hpp>

using namespace ps;

namespace ps{


        /*
                poker_stove AKs TT QQ
                poker_stove --cache new_cache.bin AKs TT
         */


        int pokerstove_driver(int argc, char** argv){
                using namespace ps::frontend;

                auto& cache_      = holdem_class_eval_cache_factory::get("main");
                std::string engine = "principal";


                std::vector<std::string> players_s;
                std::vector<frontend::range> players;


                int arg_iter = 1;

                for(; arg_iter < argc;){
                        int args_left{ argc - arg_iter };
                        switch(args_left){
                        default:
                        case 2:
                                if( argv[arg_iter] == std::string{"--engine"} ){
                                        engine = argv[arg_iter+1];
                                        arg_iter += 2;
                                        continue;
                                }
                                #if 0
                                if( argv[arg_iter] == std::string{"--cache"} ){
                                        std::cerr << "Loading...\n";

                                        if( ! calc.load(argv[arg_iter+1] ) ){
                                                std::cerr << "Failed to load " << argv[arg_iter+1] << "\n";
                                        } else {
                                                std::cerr << "Done...\n";
                                        }
                                        arg_iter += 2;
                                        continue;
                                }
                                #endif
                        case 1:
                                // we must have players now
                                for(; arg_iter < argc;++arg_iter){
                                        players_s.push_back( argv[arg_iter] );
                                        players.push_back( frontend::parse(argv[arg_iter]));
                                }
                                break;
                        }
                }

                auto& eval_       = equity_evaluator_factory::get("principal");
                auto& class_eval_ = class_equity_evaluator_factory::get(engine);

                tree_range root( players );
                root.display();

                auto agg = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
                boost::timer::auto_cpu_timer at;
                for( auto const& c : root.children ){

                        // this means it's a class vs class evaulation
                        if( c.opt_cplayers.size() != 0 ){
                                holdem_class_vector aux{c.opt_cplayers};
                                agg->append(*class_eval_.evaluate(aux));
                        } else{
                                for( auto const& d : c.children ){
                                        holdem_hand_vector aux{d.players};
                                        agg->append(*eval_.evaluate(aux));
                                }
                        }
                }

                pretty_printer(std::cout, *agg, players_s);

                return 0;
        }
} // anon

int main(int argc, char** argv){
        return pokerstove_driver(argc, argv);
}
