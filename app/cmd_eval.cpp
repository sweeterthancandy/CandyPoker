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

namespace {


void display_breakdown(std::ostream& ostr, interface_::EvaulationResultView const& view){

        using namespace Pretty;
                
        std::vector<std::string> title;


        title.emplace_back("range");
        title.emplace_back("equity");
        title.emplace_back("wins");
        for(size_t i=0; i != view.players().size() -1;++i){
                title.emplace_back("draw_"+ boost::lexical_cast<std::string>(i+1));
        }
        title.emplace_back("any draw");
        title.emplace_back("any draw norm");
        title.emplace_back("sigma");
                
        std::vector< LineItem > lines;
        lines.emplace_back(title);
        lines.emplace_back(LineBreak);

                
        for(auto const& p : view.players())
        {
                std::vector<std::string> line;

                line.emplace_back( p.Range());
                line.emplace_back( str(boost::format("%.4f%%") % (p.EquityAsDouble() * 100)));
                        
                for(auto const& nwins : p.NWinVector())
                {
                        line.emplace_back(std::to_string(nwins));
                }

                line.emplace_back(std::to_string(p.AnyDraws()));
                line.emplace_back(str(boost::format("%.2f") %(p.AnyDrawsNormalised())));
                line.emplace_back(std::to_string(p.Sigma()));

                    

                lines.push_back(line);
        }

        RenderTablePretty(std::cout, lines);
                
}

struct MaskEval : Command{
        explicit
        MaskEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                namespace bpt = boost::program_options;

                bool debug{false};
                bool step{false};
                bool times{false};
                bool help{false};
                bool emit_csv{false};
                bool instr_cache{true};
                std::vector<std::string> players_s;
                // way to choose a specific one
                std::string engine;

                bpo::options_description desc("Solver command");
                desc.add_options()
                        ("debug-instr"     , bpo::value(&debug)->implicit_value(true), "debug flag")
                        ("step"     , bpo::value(&step)->implicit_value(true), "step eval")
                        ("times"     , bpo::value(&times)->implicit_value(true), "time instructions")
                        ("help"      , bpo::value(&help)->implicit_value(true), "this message")
                        ("player"    , bpo::value(&players_s), "player ranges")
                        ("emit-csv"    , bpo::value(&emit_csv), "output csv file for inspection of transforms")
                        ("engine"     , bpo::value(&engine), "choose speicifc eval mechinism")
                        ("instr-cache"    , bpo::value(&instr_cache), "dev flag")
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



                const interface_::EvaluationObject::Flags flags = 0
                        | ( instr_cache ? interface_::EvaluationObject::F_CacheInstructions : 0 )
                        | ( debug ? interface_::EvaluationObject::F_DebugInstructions : 0 )
                        | ( step ? interface_::EvaluationObject::F_StepPercent : 0 )
                        | ( times ? interface_::EvaluationObject::F_TimeInstructionManager: 0 )
                        ;
                interface_::EvaluationObject obj(players_s, engine, flags);

                boost::timer::cpu_timer tmr;
                auto prepared = obj.Prepare();

                if (step)
                {

                        PS_LOG(trace) << tmr.format(4, "prepare took %w seconds");
                        tmr.start();
                }

                auto result_view = prepared->Compute();
                if (step)
                {
                        PS_LOG(trace) << tmr.format(4, "compute took %w seconds");
                        tmr.start();

                        std::cout << matrix_to_string(result_view.get_matrix()) << "\n";
                }
                display_breakdown(std::cout, result_view);

                return EXIT_SUCCESS;
        }       
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<MaskEval> MaskEvalDecl{"eval"};

} // end namespace anon

