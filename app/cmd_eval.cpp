/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
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

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

using namespace ps;

namespace{

struct MaskEval : Command{
        explicit
        MaskEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                namespace bpt = boost::program_options;

                bool debug{false};
                bool help{false};
                std::vector<std::string> players_s;
                // way to choose a specific one
                std::string engine;

                bpo::options_description desc("Solver command");
                desc.add_options()
                        ("debug"     , bpo::value(&debug)->implicit_value(true), "debug flag")
                        ("help"      , bpo::value(&help)->implicit_value(true), "this message")
                        ("player"    , bpo::value(&players_s), "player ranges")
                        ("engine"     , bpo::value(&engine), "choose speicifc eval mechinism")
                ;

                bpo::positional_options_description pd;
                pd.add("player", -1);

                bpo::variables_map vm;
                bpo::store(bpo::command_line_parser(args_)
                           .options(desc)
                           .positional(pd)
                           .run(), vm);
                bpo::notify(vm);    



                if( help ){
                        std::cout << desc << "\n";
                        return EXIT_SUCCESS;
                }

                std::vector<frontend::range> players;
                for(auto const& s : players_s ){
                        players.push_back( expand(frontend::parse(s)) );
                }

                computation_context comp_ctx{players.size()};

                computation_pass_manager mgr;
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_permutate>();
                mgr.add_pass<pass_sort_type>();
                mgr.add_pass<pass_collect>();
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_eval_hand_instr_vec>(engine);
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_write>();

                boost::timer::auto_cpu_timer at;

                #if 0
                instruction_list instr_list = frontend_to_instruction_list(players);
                auto result = mgr.execute_old(&comp_ctx, &instr_list);

                if( result ){
                        pretty_print_equity_breakdown_mat(std::cout, *result, args_);
                }
                #endif
                computation_result result{comp_ctx};
                std::string tag = "B";
                auto const& m = result.allocate_tag(tag);
                instruction_list instr_list = frontend_to_instruction_list(tag, players);
                mgr.execute_(&comp_ctx, &instr_list, &result);

                if( result ){
                        pretty_print_equity_breakdown_mat(std::cout, m, players_s);
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<MaskEval> MaskEvalDecl{"eval"};

} // end namespace anon

