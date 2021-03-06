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
#include "ps/base/cards.h"
#include "ps/support/command.h"
#include "ps/detail/print.h"
#include "ps/eval/class_cache.h"
#include "ps/eval/holdem_class_vector_cache.h"
#include "app/pretty_printer.h"
#include "app/serialization_util.h"
#include "ps/detail/graph.h"

#include "ps/sim/computer.h"
#include "ps/sim/game_tree.h"
#include "ps/sim/computer_factory.h"
#include "ps/sim/_extra.h"
#include "ps/sim/solver.h"

#include <boost/any.hpp>
#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include <numeric>


#include <boost/timer/timer.hpp>

/*
        My idea here is that I want to find a solution S, subject to certain containts,
        such as only having a single cid having a mixed solution.
        
        
        For a n-player push/fold game, we seek a solution S, which represents how each
        player plays each hand in any given situation. We know that a solution exist,
        and in fact a solution exists with only one mixed cid.
                The solution S, has the property that no player can unilaterally improve
        their ev, ie
                                EV[T] <= EV[S] \forall T ,
        for any given player. The (mixed) numerical solution, which is mostly trivial
        is just the iterative solution
                     S_{i+1} = S_{i} * \alpha + Counter(S_{i}) * ( 1 - \alpha ),
        and we can impose the stoppage condition
                        || EV[S_{i}] - EV[Counter(S_{i-1})] ||_{\inf} < \epsilon.
        However the above design will converge to a solution with multiple cid's having
        mixed solutions.
                We can modify the pervious design to have a forcing term, with the idea
        is that there is a set B of cid's for which have very little difference between
        push or fold. By introducing a forcing term, we can have the counterstrategy
        that requires a minimal jump in ev for go from push to fold. In this same design,
        we also want to map those very close to one or zero, into one or zero.
                However not all numerical solution will converge to the most restrictive
        conditon, so we need to have versile stoppage conditions.
                

                A) Mixed solution, minimal EV
                B) minimal mixed solution, 
                B) algebraic solution


                When using the EV stoppage condition, not all solution will converge, as
        there may be an endless loop of two cards ocilating.
        


                The implementation is that we have a path of solutions, that we follow
                        P0 -> P1 -> P2 -> ... -> PN,
                which each depend on the solution. For example with a certain parameter
                set we might converge to a solution S, but we then want to reduce the 
                parameters ( forcing or otherwise), so that we geta reduced solution.
                        We need this framework, because we can't always find an ideal 
                solution, but looking for an idea solition we normally find increasingly
                better solutions,
                        M13 < M12 < M11 < ... M1,
                which means that each decisoion is mixed for only N cid's. We know we 
                can't have more than 13 mixed, as we assume that each run of the from
                X,X+k is monotone, which means that
                        f(42o) <= f(53o) <= ... <= f(KQo) <= f(AQo).

                        We also have a metric for how good a soltion is,
                                EvDiff() -> how much the solutions counter strategy differs
                                Gamma() -> how many cards differ with the counter strategy.

 */



namespace ps{
        using namespace sim;
        


        struct ContextImpl : SolverContext{
                // A large part of the solution is being able to run it for 6 hours,
                // then turn off the computer, and come back to it at a later date
                virtual void Message(std::string const& msg){
                        std::cerr << msg << "\n";
                }
        };



        struct RequestHandlerResult{
                bool handled;
                boost::optional<StateType> S;
        };
        struct RequestHandler{
                virtual ~RequestHandler()=default;
                virtual RequestHandlerResult HandleRequest(std::string const& solver,
                                                           std::shared_ptr<GameTree> gt,
                                                           LazyComputer AG,
                                                           StateType const& inital_state,
                                                           std::string const& solver_extra)=0;
        };
        struct UnderlyingRequestHandler : RequestHandler{
                virtual RequestHandlerResult HandleRequest(std::string const& solver_name,
                                                           std::shared_ptr<GameTree> gt,
                                                           LazyComputer AG,
                                                           StateType const& inital_state,
                                                           std::string const& solver_extra)override
                {
                        auto solver = SolverDecl::MakeSolver(solver_name, solver_extra);
                        ContextImpl ctx;
                        auto opt = solver->Execute(ctx, gt, AG.Get(), inital_state);
                        return {true, std::move(opt)};
                }
        };
        struct CacheRequestHandler : RequestHandler{
                CacheRequestHandler(std::string const& cache_name,
                                    std::shared_ptr<RequestHandler> handler)
                        : handler_{handler}
                {
                        ss_.try_load_or_default(cache_name);
                }
                virtual RequestHandlerResult HandleRequest(std::string const& solver_name,
                                                           std::shared_ptr<GameTree> gt,
                                                           LazyComputer AG,
                                                           StateType const& inital_state,
                                                           std::string const& solver_extra)override
                {
                        std::stringstream encoding;
                        encoding << gt->StringDescription() << "@"
                                 << solver_name 
                                 << solver_extra;
                        auto key = encoding.str();
                        auto iter = ss_.find(key);
                        if( iter != ss_.end()){
                                auto S = iter->second.to_eigen_vv();
                                if( S == StateType{} )
                                        return { true, {} };
                                return { true, std::move(S) };
                        }

                        auto result = handler_->HandleRequest(solver_name, gt, AG, inital_state, solver_extra);

                        if( ! result.S ){
                                // even without a solution, we have to failure 
                                // so that we don't waste time 
                                ss_.add_solution(key, StateType{});
                        } else {
                                ss_.add_solution(key, result.S.get());
                        }
                        ss_.save_();

                        return {true, result.S};
                }
                void Display()const{
                        for(auto const& sol : ss_ ){
                                std::cout << "sol.first => " << sol.first << "\n"; // __CandyPrint__(cxx-print-scalar,sol.first)
                                auto vv = sol.second.to_eigen_vv();
                        }
                }
        private:
                holdem_binary_solution_set_s ss_;
                std::shared_ptr<RequestHandler> handler_;
        };

        struct SolutionPrinter{
                explicit SolutionPrinter(size_t sub_dp = 2)
                        :sub_dp_(sub_dp)
                {}
                void operator()( std::shared_ptr<GameTree> gt,
                                 LazyComputer const& AG,
                                 boost::optional<StateType> const& opt)
                {
                        if( ! opt )
                                return;
                        pretty_print_strat(opt.get()[0][0], sub_dp_);
                        pretty_print_strat(opt.get()[1][0], sub_dp_);
                }
        private:
                size_t sub_dp_;
        };
        struct TableSequenceObserver{
                TableSequenceObserver(){
                        conv_tb.push_back(std::vector<std::string>{
                                "Desc", "?", "Level", "Total", "Gamma", "GammaCards",
                                "Mixed", "MixedCards", "|.|"});

                        conv_tb.push_back(Pretty::LineBreak);
                }
                void operator()( std::shared_ptr<GameTree> gt,
                                 LazyComputer const& AG,
                                 boost::optional<StateType> const& opt)
                {
                        if( ! opt ){
                                conv_tb.push_back(std::vector<std::string>{gt->StringDescription(), "no"});
                        } else {
                                auto sol = sim::Solution::MakeWithDeps(gt, AG.Get(), *opt);

                                std::vector<std::string> line;
                                line.push_back(gt->StringDescription());
                                line.push_back("yes");
                                line.push_back(boost::lexical_cast<std::string>(sol.Level));
                                line.push_back(boost::lexical_cast<std::string>(sol.Total));
                                line.push_back(detail::to_string(sol.Gamma));
                                line.push_back(detail::to_string(sol.GammaCards));

                                line.push_back(detail::to_string(sol.Mixed));
                                line.push_back(detail::to_string(sol.MixedCards));
                                line.push_back(boost::lexical_cast<std::string>(sol.Norm));

                                conv_tb.push_back(std::move(line));
                        }
                        Pretty::RenderTablePretty(std::cout, conv_tb);
                }
        private:
                std::vector<Pretty::LineItem> conv_tb;
        };
        struct TableCollater{
                enum { Dp = 1 };
                ~TableCollater(){
                        if( ! last_gt_ ) 
                                return;
                        Display();
                }
                void operator()( std::shared_ptr<GameTree> gt,
                                 LazyComputer const& AG,
                                 boost::optional<StateType> const& opt)
                {
                        if( ! opt )
                                return;
                        for(; S.size() < opt->size();){
                                S.emplace_back();
                                S.back().resize(169);
                                S.back().fill(0.0);
                        }
                        for(size_t j=0;j!=opt->size();++j){
                                for(size_t idx=0;idx!=169;++idx){
                                        if( std::fabs(opt.get()[j][0][idx] - 1.0 ) < 1e-2 ){
                                                S[j][idx] = gt->EffectiveStack();
                                        }
                                }
                        }
                        last_gt_ = gt;

                        Display();
                }
                void Display()const{
                        for( auto const& _ : *last_gt_){
                                std::cout << "\n            " << _.PrettyAction() << "\n\n";
                                pretty_print_strat(S[_.GetIndex()], Dp);

                        }
                }
        private:
                std::vector<Eigen::VectorXd> S;
                std::shared_ptr<GameTree> last_gt_;
        };

        
        struct SolverCmd : Command{
                enum{ Debug = 1};
                explicit
                SolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{


                        bool debug{false};
                        double sb = 0.5;
                        double bb = 1.0;
                        double eff_lower = 10;
                        double eff_upper = 10;
                        double eff_inc = 1.0;
                        std::string game_tree = "two-player-push-fold";
                        std::string solver_s = "simple-numeric";
                        std::string extra;
                        bool help{false};
                        bool memory{false};
                        size_t sub_dp = 1;
                        std::string req_handler_name = "underlying";
                        bool cum_table{false};
                        bool print_seq{false};


                        bpo::options_description desc("Solver command");
                        desc.add_options()
                                ("debug"     , bpo::value(&debug)->implicit_value(true), "debug flag")
                                ("help"      , bpo::value(&help)->implicit_value(true), "this message")
                                ("eff-lower" , bpo::value(&eff_lower), "lower limit for range")
                                ("eff-upper" , bpo::value(&eff_upper), "upper limit for range")
                                ("eff-inc"   , bpo::value(&eff_inc), "incremnt for eff stack")
                                ("solver"    , bpo::value(&solver_s), "specigic solver")
                                ("game-tree" , bpo::value(&game_tree), "game tree")
                                ("extra"     , bpo::value(&extra), "extra options for the specigfic solver")
                                ("memory" , bpo::value(&memory)->implicit_value(true), "cache results")
                                ("cum-table" , bpo::value(&cum_table)->implicit_value(true), "produce cumulative table")
                                ("sub-dp"    , bpo::value(&sub_dp) , "")
                                ("print-seq" , bpo::value(&print_seq)->implicit_value(true), "print table with solution information")
                        ;



                        std::vector<const char*> aux;
                        aux.push_back("dummy");
                        for(auto const& _ : args_){
                                aux.push_back(_.c_str());
                        }
                        aux.push_back(nullptr);

                        bpo::variables_map vm;
                        bpo::store(parse_command_line(aux.size()-1, &aux[0], desc), vm);
                        bpo::notify(vm);    



                        if( help ){
                                std::cout << desc << "\n";
                                return EXIT_SUCCESS;
                        }

                

                        std::shared_ptr<GameTree> any_gt;


                        auto undlying_req_handler = std::make_shared<UnderlyingRequestHandler>();
                        std::shared_ptr<RequestHandler> req_handler = undlying_req_handler;
                        if( memory ){
                                req_handler = std::make_shared<CacheRequestHandler>(".ps.request_cache", undlying_req_handler);
                        }
                        //req_handler->Display();

                        boost::optional<StateType> guess;

                        std::vector<std::function<void(std::shared_ptr<GameTree>, LazyComputer const&, boost::optional<StateType> const&)> > obs;

                        obs.push_back(SolutionPrinter(sub_dp));
                        if( print_seq ){
                                obs.push_back(TableSequenceObserver());
                        }
                        if( cum_table ){
                                obs.push_back(TableCollater());
                        }


                        for(double eff = eff_lower;eff - 1e-4 < eff_upper; eff += eff_inc ){

                                std::shared_ptr<GameTree> gt;

                                if( game_tree == "two-player-push-fold" ){
                                        gt = std::make_shared<GameTreeTwoPlayer>(sb, bb, eff);
                                } else if ( game_tree == "three-player-push-fold" ){
                                        gt = std::make_shared<GameTreeThreePlayer>(sb, bb, eff);
                                } else if ( game_tree == "two-player-raise-fold" ){
                                        gt = std::make_shared<GameTreeTwoPlayerRaiseFold>(sb, bb, eff, 2.0);
                                } else{
                                        BOOST_THROW_EXCEPTION(std::domain_error("no game tree called " + game_tree));
                                }
                                any_gt = gt;

                                LazyComputer AG{gt};


                                StateType inital_state;
                                if( guess ){
                                        inital_state = *guess;
                                } else {
                                        inital_state = gt->MakeDefaultState();
                                }
                                
                                auto result = req_handler->HandleRequest(solver_s, gt, AG, inital_state, extra);
                                auto& opt = result.S;

                                if( opt ){
                                        guess = *opt;
                                }
                                for(auto& ob : obs ){
                                        ob(gt, AG, opt);
                                }

                        }

                        for(;obs.size();obs.pop_back());

                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<SolverCmd> SolverCmdDecl{"solver"};
} // end namespace ps
