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


#include "ps/support/config.h"
#include "ps/support/command.h"
#include "app/pretty_printer.h"

#if 0
#include "ps/base/cards.h"
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
#endif

#include "ps/support/persistent.h"
#include "ps/eval/holdem_class_vector_cache.h"
#include "ps/interface/interface.h"


#include <Eigen/Dense>
#include <fstream>


#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>
#include <boost/log/trivial.hpp>



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


#if 0
                if (breakdown)
                {
                    const size_t num_players = players_s.size();

                    std::vector<frontend::range> players;
                    for (auto const& s : players_s) {
                        players.push_back(frontend::parse(s));
                    }

                    struct SubComputation
                    {
                        std::vector<holdem_class_id> opt_class_vec;
                        std::vector<holdem_id> player_hand_vec;
                        std::vector<std::string> player_hand_str;
                    };
                    std::vector<SubComputation> subs;

                    tree_range root(players);
                    for (auto const& c : root.children) {
                        for (auto const& d : c.children) {
                            std::vector<std::string> aux_as_str;
                            for (auto const& id : d.players)
                            {
                                aux_as_str.push_back(holdem_hand_decl::get(id).to_string());
                            }
                            subs.push_back(SubComputation{
                                c.opt_cplayers,
                                d.players,
                                aux_as_str });
                        }
                    }

                    subs.push_back(SubComputation{
                        {},
                        {},
                        players_s });
                

                    std::vector<std::vector<std::string>> tmp;
                    for (auto const& x : subs)
                    {
                        std::cout << std_vector_to_string(x.player_hand_str) << "\n";
                        tmp.push_back(x.player_hand_str);
                    }
                    auto result_list_view = interface_::evaluate_list(tmp, engine);

                    std::vector<std::string> title;


                    title.emplace_back("i"); // breakdown index
                    title.emplace_back("p"); // player index
                    title.emplace_back("class");
                    title.emplace_back("hand");
                    title.emplace_back("equity");

                    using namespace Pretty;
                    std::vector< LineItem > lines;
                    lines.push_back(title);

                    for (size_t idx = 0; idx != subs.size(); ++idx)
                    {
                        auto const& sub = subs[idx];
                        auto const& result_view = result_list_view[idx];
                        for (size_t player_index = 0; player_index != num_players; ++player_index)
                        {
                            auto const& player_view = result_view.player_view(player_index);

                            const auto class_fmt = [&]()->std::string {
                                if (sub.opt_class_vec.size()) {
                                    return holdem_class_decl::get(sub.opt_class_vec.at(player_index)).to_string();
                                }
                                return "";
                            }();

                            const auto equity_fmt = str(boost::format("%.4f%%") % (player_view.EquityAsDouble() * 100));

          
                            lines.push_back(
                                std::vector<std::string>{
                                    std::to_string(idx),
                                    std::to_string(player_index),
                                    class_fmt,
                                    sub.player_hand_str.at(player_index),
                                    equity_fmt });
                            
                        }
                    }
                    RenderTablePretty(std::cout, lines);
                }
                else
                {
                    auto result_view = interface_::evaluate(players_s, engine);
                    pretty_print_equity_breakdown_mat(std::cout, result_view.get_matrix(), players_s);
                }
#endif


                interface_::EvaluationObject obj(players_s, engine, debug);
                auto result_view = obj.Compute();
                pretty_print_equity_breakdown_mat(std::cout, result_view.get_matrix(), players_s);

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<MaskEval> MaskEvalDecl{"eval"};

} // end namespace anon

