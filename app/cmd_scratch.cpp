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
                ContextImpl(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG)
                        :gt_(gt),
                        AG_(AG)
                {
                        //ss_.try_load_or_default(".ps.context.ss");
                }


                virtual std::shared_ptr<GameTree>         ArgGameTree()override{ return gt_; }
                virtual GraphColouring<AggregateComputer>& ArgComputer()override{ return AG_; }
                virtual std::vector<std::string>           ArgExtra()override{
                        return args_;
                }

                // A large part of the solution is being able to run it for 6 hours,
                // then turn off the computer, and come back to it at a later date
                virtual void Message(std::string const& msg){
                        std::cerr << msg << "\n";
                }
                virtual void UpdateCandidateSolution(StateType const& S){
                        enum{ Dp = 10};
                        //DisplayStrategy(S, Dp);

                        ss_.add_solution(UniqeKey(), S);
                        ss_.save_();

                }
                virtual boost::optional<StateType> RetreiveCandidateSolution(){
                        auto iter = ss_.find(UniqeKey());
                        if( iter != ss_.end()){
                                return iter->second.to_eigen_vv();
                        }
                        return {};
                }

                virtual void EmitSolution(StateType const& S){
                        BOOST_ASSERT( ! S_ );
                        S_ = S;
                        ss_.remove_solution(UniqeKey());
                        ss_.save_();
                }
                auto const& Get()const{ return S_; }

                void AddArg(std::string const& s){
                        args_.push_back(s);
                }
        private:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer> AG_;
                boost::optional<StateType> S_;
                holdem_binary_solution_set_s ss_;
                std::vector<std::string> args_;
        };




        
        struct ScratchCmd : Command{
                enum{ Debug = 1};
                explicit
                ScratchCmd(std::vector<std::string> const& args):args_{args}{}
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


                        bpo::options_description desc("Scratch command");
                        desc.add_options()
                                ("debug"     , bpo::value(&debug)->implicit_value(true), "debug flag")
                                ("help"      , bpo::value(&help)->implicit_value(true), "this message")
                                ("eff-lower" , bpo::value(&eff_lower), "lower limit for range")
                                ("eff-upper" , bpo::value(&eff_upper), "upper limit for range")
                                ("eff-inc"   , bpo::value(&eff_inc), "incremnt for eff stack")
                                ("solver"    , bpo::value(&solver_s), "specigic solver")
                                ("game-tree" , bpo::value(&game_tree), "game tree")
                                ("extra"     , bpo::value(&extra), "extra options for the specigfic solver")
                                ("memory"    , bpo::value(&memory)->implicit_value(true), "cache results")
                        ;



                        std::vector<const char*> aux;
                        aux.push_back("dummy");
                        for(auto const& _ : args_){
                                std::cout << "_ => " << _ << "\n"; // __CandyPrint__(cxx-print-scalar,_)
                                aux.push_back(_.c_str());
                        }
                        aux.push_back(nullptr);

                        bpo::variables_map vm;
                        bpo::store(parse_command_line(aux.size()-1, &aux[0], desc), vm);
                        bpo::notify(vm);    



                        if( help ){
                                std::cout << desc << "\n";
                                return EXIT_FAILURE;
                        }

                
                        holdem_binary_solution_set_s ss;
                        if( memory ){
                                ss.try_load_or_default(".ss.bin");
                        }
                        
                        enum { Dp = 1 };

                        std::vector<Eigen::VectorXd> S;

                        std::shared_ptr<GameTree> any_gt;

                        std::vector<Pretty::LineItem> conv_tb;
                        conv_tb.push_back(std::vector<std::string>{"Desc", "?"});
                        conv_tb.push_back(Pretty::LineBreak);

                        for(double eff = eff_lower;eff - 1e-4 < eff_upper; eff += eff_inc ){

                                std::shared_ptr<GameTree> gt;

                                if( game_tree == "two-player-push-fold" ){
                                        gt = std::make_shared<GameTreeTwoPlayer>(sb, bb, eff);
                                } else if ( game_tree == "three-player-push-fold" ){
                                        gt = std::make_shared<GameTreeThreePlayer>(sb, bb, eff);
                                } else{
                                        BOOST_THROW_EXCEPTION(std::domain_error("no game tree called " + game_tree));
                                }
                                any_gt = gt;
                                #if 0
                                auto opt = dvr.FindOrBuildSolution(gt, solver, extra);
                                #endif

                                GraphColouring<AggregateComputer> AG = MakeComputer(gt);


                                Solver* solver = Solver::Get(solver_s);
                                if( ! solver ){
                                        BOOST_THROW_EXCEPTION(std::domain_error("solver doesn't exist " + solver_s));
                                }

                                ContextImpl ctx{gt, AG};
                                if( extra.size())
                                        ctx.AddArg(extra);
                                solver->Execute(ctx);
                                auto opt = ctx.Get();


                                auto root   = gt->Root();
                                auto opt_s = ( opt ? "yes" : "no" );
                                conv_tb.push_back(std::vector<std::string>{gt->StringDescription(), opt_s});
                                if( opt ){
                                        pretty_print_strat(opt.get()[0][0], 1);
                                        pretty_print_strat(opt.get()[1][0], 1);
                                        for(; S.size() < opt->size();){
                                                S.emplace_back();
                                                S.back().resize(169);
                                                S.back().fill(0.0);
                                        }
                                        for(size_t j=0;j!=opt->size();++j){
                                                for(size_t idx=0;idx!=169;++idx){
                                                        if( std::fabs(opt.get()[j][0][idx] - 1.0 ) < 1e-2 ){
                                                                S[j][idx] = eff;
                                                        }
                                                }
                                        }
                                }
                                Pretty::RenderTablePretty(std::cout, conv_tb);
                        }
                        for( auto const& _ : *any_gt){
                                std::cout << "\n            " << _.PrettyAction() << "\n\n";
                                pretty_print_strat(S[_.GetIndex()], Dp);

                        }

                        Pretty::RenderTablePretty(std::cout, conv_tb);
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<ScratchCmd> ScratchCmdDecl{"scratch"};
} // end namespace ps
