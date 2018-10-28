#ifndef PS_CMD_BETTER_SOLVER_H
#define PS_CMD_BETTER_SOLVER_H

#include <thread>
#include <numeric>
#include <atomic>
#include <bitset>
#include <fstream>
#include <unordered_map>

#include <boost/format.hpp>
#include <boost/assert.hpp>

#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/base/cards.h"
#include "ps/base/cards.h"
#include "ps/base/frontend.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/tree.h"

#include "ps/detail/tree_printer.h"

#include "ps/eval/class_cache.h"
#include "ps/eval/pass_mask_eval.h"
#include "ps/eval/instruction.h"

#include "ps/support/config.h"
#include "ps/support/index_sequence.h"

#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>

#include "ps/support/command.h"

#include <boost/iterator/indirect_iterator.hpp>
#include "ps/eval/holdem_class_vector_cache.h"
#include "ps/eval/binary_strategy_description.h"


namespace ps{

        #if 0
        struct dev_class_cache_item{
                holdem_class_vector cv;
                std::vector<double> ev;
        };
        inline
        size_t hash_value(dev_class_cache_item const& item){
                return boost::hash_range(item.cv.begin(), item.cv.end());
        }
        #endif


        inline Eigen::VectorXd choose_push_fold(Eigen::VectorXd const& push, Eigen::VectorXd const& fold){
                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }
                return result;
        }
        inline Eigen::VectorXd clamp(Eigen::VectorXd s){
                for(holdem_class_id idx=0;idx!=169;++idx){
                        s(idx) = ( s(idx) < .5 ? .0 : 1. );
                }
                return s;
        }


        struct SolverCmd : Command{
                enum{ Debug = 1};
                enum{ Dp = 2 };
                explicit
                SolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        std::unique_ptr<binary_strategy_description> desc;

                        if( args_.size() && args_[0] == "three"){
                                desc = binary_strategy_description::make_three_player_description(0.5, 1, 10);
                        } else{
                                desc = binary_strategy_description::make_hu_description(0.5, 1, 10);
                        }


                        auto state0 = desc->make_inital_state();

                        auto state = state0;

                        double factor = 0.10;


                        using namespace Pretty;
                        std::vector<LineItem> lines;
                        size_t line_idx = 0;
                        std::vector<std::string> header;
                        header.push_back("n");
                        for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                header.push_back(si->action());
                        }
                        header.push_back("max");
                        for(size_t idx=0;idx!=desc->num_players();++idx){
                                std::stringstream sstr;
                                sstr << "ev[" << idx << "]";
                                header.push_back(sstr.str());
                        }
                        header.push_back("time");
                        lines.push_back(std::move(header));
                        lines.push_back(LineBreak);

                        boost::timer::cpu_timer timer;


                        using state_type = binary_strategy_description::strategy_impl_t;

                        auto step = [&](auto const& state)->state_type
                        {
                                using result_t = std::future<std::tuple<size_t, Eigen::VectorXd> >;
                                std::vector<result_t> tmp;
                                for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                        auto fut = std::async(std::launch::async, [&,si](){
                                                auto fold_s = si->make_all_fold(state);
                                                auto fold_ev = desc->expected_value_by_class_id(si->player_index(), fold_s);

                                                auto push_s = si->make_all_push(state);
                                                auto push_ev = desc->expected_value_by_class_id(si->player_index(), push_s);

                                                auto counter= choose_push_fold(push_ev, fold_ev);
                                                return std::make_tuple(si->vector_index(), counter);
                                        });
                                        tmp.emplace_back(std::move(fut));
                                }
                                auto result = state;
                                for(auto& _ : tmp){
                                        auto ret = _.get();
                                        auto idx            = std::get<0>(ret);
                                        auto const& counter = std::get<1>(ret);
                                        result[idx] = state[idx] * ( 1.0 - factor ) + counter * factor;
                                }
                                return result;
                        };
                        

                        auto stoppage_condition = [&](state_type const& from, state_type const& to)->bool
                        {
                                std::vector<double> norm_vec;
                                for(size_t idx=0;idx!=from.size();++idx){
                                        auto delta = from[idx] - to[idx];
                                        auto norm = delta.lpNorm<1>();
                                        norm_vec.push_back(norm);
                                }
                                auto norm = *std::max_element(norm_vec.begin(),norm_vec.end());
                                
                                std::vector<std::string> line;
                                line.push_back(boost::lexical_cast<std::string>(line_idx));
                                for(size_t idx=0;idx!=norm_vec.size();++idx){
                                        line.push_back(boost::lexical_cast<std::string>(norm_vec[idx]));
                                }

                                
                                line.push_back(boost::lexical_cast<std::string>(norm));
                                auto ev = desc->expected_value(to);
                                for(size_t idx=0;idx!=desc->num_players();++idx){
                                        line.push_back(boost::lexical_cast<std::string>(ev[idx]));
                                }
                                line.push_back(format(timer.elapsed(), 2, "%w(%t cpu)"));
                                lines.push_back(std::move(line));
                                ++line_idx;

                                RenderTablePretty(std::cout, lines);

                                double epsilon = 0.05;
                                return norm < epsilon;
                        };
                        
                        auto print = [&]()
                        {
                                for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                        std::cout << si->description() << "\n";
                                        pretty_print_strat(state[si->vector_index()], Dp);
                                }
                        };

                        for(;;){
                                timer.start();
                                auto next = step(state);
                                timer.stop();

                                if( stoppage_condition(state, next) )
                                        break;

                                state = next;
                                if(Debug) print();
                        }
                        for(auto& _ : state){
                                _ = clamp(_);
                                
                        }
                        print();
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<SolverCmd> SolverCmdDecl{"solver"};
} // end namespace ps

#endif // PS_CMD_BETTER_SOLVER_H
