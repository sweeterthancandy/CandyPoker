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
#include <boost/any.hpp>

#include "CandyTransform/Transform.h"
namespace ps{
        namespace ct = CandyTransform;
} // end namespace ps

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


        /*
         * This is a wrapper around StateType. This solves the problem
         * of computing the counter strategy and EV in observer functions.
         */
        struct Solution{
                /*
                 * This represents the strategy
                 */
                StateType S;
                Eigen::VectorXd EV;
                /*
                 * This is one of the classifications of the
                 * soltion, which represents how many cid's 
                 * are mixed soltions (not 0 or 1).
                 */
                std::vector<size_t> Mixed;
                /*
                 * This represents how many cid's are different
                 * from the counter strategy. This would never be
                 * all zero, but ideally all 1's, which is the manifestation
                 * of the optional solution being mixed in one card
                 */
                std::vector<size_t> Gamma;
                /*
                 * Because it's expensive for calculate
                 * the Counter strategy, we keep it here on
                 * the object rather than do the calculation
                 * multiple times
                 */
                StateType Counter;
                Eigen::VectorXd counter_EV;
        };

        






        void DisplayStrategy(StateType const& S){
                for(size_t idx=0;idx!=S.size();++idx){
                        pretty_print_strat(S[idx][0], 4);
                }
        }


        #if 0
        boost::optional<StateType> NumericalSolver(holdem_binary_strategy_ledger_s& ledger, std::shared_ptr<GameTree> gt,
                                                   GraphColouring<AggregateComputer>& AG)
        {


                auto root   = gt->Root();
                auto state0 = gt->InitialState();




                decltype(gt->MakeDefaultState()) S0;
                if( ledger.size()){
                        S0 = ledger.back().to_eigen_vv();
                } else {
                        S0 = gt->MakeDefaultState();
                }

                auto S = S0;

                auto N      = gt->NumPlayers();

                enum{ MaxIter = 500 };

                double delta = 0.01 / 4;
                
                std::vector<std::shared_ptr<AnyObserver> > obs;
                auto mixed = std::make_shared<MinMixedSolutionCondition>(gt, AG);
                obs.push_back(mixed);
                obs.push_back(std::make_shared<NonMixedSolutionSolutionCondition>(gt, AG));
                obs.push_back(std::make_shared<SolutionPrinter>());

                size_t k=0;
                for(;k!=9;++k){
                        GeometricLoopOptions opts;
                        opts.Delta = delta;
                        auto solution = GeometricLoopWithClamp(opts,
                                                               gt,
                                                               AG,
                                                               S,
                                                               obs);
                        if( ! solution ){
                                return {};
                        }
                        ledger.push(solution.S);
                        ledger.save_();
                        if( auto ptr = boost::any_cast<NonMixedSolutionSolution>(&solution.Category)){
                                // we have a converged solution, but it's not minimal mixed
                                delta /= 2.0;
                                S = solution.S;
                        }
                        else if( auto ptr = boost::any_cast<MinMixedSolution>(&solution.Category)){
                                return solution.S;
                        } else{
                                // must not converg
                                break;
                        }

                }

                if( auto ms = mixed->Get() ){
                        return ms->S;
                }
                return {};
        }
        boost::optional<StateType> AlgebraicSolver(holdem_binary_strategy_ledger_s& ledger, std::shared_ptr<GameTree> gt,
                                                   GraphColouring<AggregateComputer>& AG)
        {
                auto numerical_solution = NumericalSolver(ledger, gt, AG);


                return numerical_solution;

        }
        #endif

        struct SolverConcept{
                virtual ~SolverConcept()=default;
                virtual boost::optional<StateType> ComputeSolution(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer>& AG)const noexcept=0;
        };


        struct NumericalSolver : SolverConcept{
                virtual boost::optional<StateType> ComputeSolution(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer>& AG)const noexcept override
                {
                        double factor_ = 0.05;
                        double epsilon = 0.00001;
                        enum{ Stride = 10 };
                        auto root   = gt->Root();
                        auto state0 = gt->MakeDefaultState();
                        
                        auto S = state0;
                        for(size_t counter=0;counter!=400;){
                                for(size_t inner=0;inner!=Stride;++inner, ++counter){
                                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, 0.0);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - factor_ );
                                }
                                auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, 0.0);

                                auto ev = AG.Color(root).ExpectedValue(S);
                                auto counter_ev = AG.Color(root).ExpectedValue(S_counter);
                                auto d = ev - counter_ev;
                                auto norm = d.lpNorm<Eigen::Infinity>();
                                std::cout << "norm = " << norm << "\n";
                                DisplayStrategy(S);
                                if( norm < epsilon ){
                                        return S;
                                }
                        }
                        return {};
                }
        };
        namespace SolverPaths{

                struct Context{
                        std::shared_ptr<GameTree> gt;
                        GraphColouring<AggregateComputer>& AG;

                        StateType S;
                        GNode* root;
                        double Epsilon{1e-8};
                };

                struct StaticLoop : ct::Transform<std::shared_ptr<Context>, std::shared_ptr<Context> >{
                        size_t Count{10};
                        double Factor{0.05};

                        virtual void Apply(ct::TransformControl* ctrl, ParamType in)override{
                                for(size_t n=0;n!=Count;++n){
                                        auto S_counter = computation_kernel::CounterStrategy(in->gt, in->AG, in->S, 0.0);
                                        computation_kernel::InplaceLinearCombination(in->S, S_counter, 1 - Factor );
                                }
                                ctrl->Pass();
                        }
                };

                struct MaybeStop : ct::Transform<std::shared_ptr<Context>, std::shared_ptr<Context> >{
                        virtual void Apply(ct::TransformControl* ctrl, ParamType in)override
                        {
                                auto S_counter = computation_kernel::CounterStrategy(in->gt, in->AG, in->S, 0.0);
                                auto ev = in->AG.Color(in->root).ExpectedValue(in->S);
                                auto counter_ev = in->AG.Color(in->root).ExpectedValue(S_counter);
                                auto d = ev - counter_ev;
                                auto norm = d.lpNorm<Eigen::Infinity>();

                                std::cout << "norm = " << norm << "\n";
                                DisplayStrategy(in->S);

                                if( norm < in->Epsilon ){
                                        ctrl->Return(in->S);
                                } else {
                                        ctrl->Pass();
                                }
                        }
                };



        } // end namespace SolverPaths
        struct CTNumericalSolver : SolverConcept{
                struct LoopInit
                        : ct::Transform<std::shared_ptr<SolverPaths::Context>, std::shared_ptr<SolverPaths::Context> >
                {
                        virtual void Apply(ct::TransformControl* ctrl, ParamType in)override
                        {
                                for(double epsilon = 1e-4; epsilon > 1e-20; epsilon /= 10){
                                        auto ctx = std::make_shared<SolverPaths::Context>(*in);
                                        ctx->Epsilon = epsilon;
                                        ctrl->Emit(ctx);
                                }
                        }
                };
                struct MainLoop
                        : ct::Transform<std::shared_ptr<SolverPaths::Context>
                        , std::shared_ptr<SolverPaths::Context> >, std::enable_shared_from_this<MainLoop>
                {
                        using StaticLoop = SolverPaths::StaticLoop; 
                        using MaybeStop  = SolverPaths::MaybeStop;
                        virtual void Apply(ct::TransformControl* ctrl, ParamType in)override
                        {
                                std::cout << "ctrl->Depth() = " << ctrl->Depth() << "\n";
                                if( ctrl->Depth() < 100 ){
                                        auto dp = ctrl->DeclPath();
                                        dp->Next(A)
                                          ->Next(B)
                                          ->Next(shared_from_this());
                                        ctrl->Pass();
                                }
                        }
                        std::shared_ptr<StaticLoop> A{std::make_shared<StaticLoop>()};
                        std::shared_ptr<MaybeStop> B{std::make_shared<MaybeStop>()};
                };
                virtual boost::optional<StateType> ComputeSolution(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer>& AG)const noexcept override
                {
                        using namespace SolverPaths;
                        ct::TransformContext ctx;
                        auto path = ctx.Start();
                        path->Next(std::make_shared<LoopInit>())
                            ->Next(std::make_shared<MainLoop>())
                        ;
                        
                        using SolverPaths::Context;
                        auto pp = std::make_shared<Context>(Context{gt, AG});
                        pp->S = gt->MakeDefaultState();
                        pp->root = gt->Root();
                        std::vector<StateType> SV = ctx.Execute<StateType>(pp);
                        std::cout << "SV.size() = " << SV.size() << "\n";
                        if( SV.size() ){
                                return SV.back();
                        }
                        return {};
                }
        };

        struct Driver{
                // enable this for debugging
                enum{ NoPersistent = true };
                Driver(){
                        if( ! NoPersistent ){
                                ss_.try_load_or_default(".ss.bin");
                        }
                }
                ~Driver(){
                        // will do nothing when no save
                        ss_.save_();
                }
                boost::optional<std::vector<std::vector<Eigen::VectorXd> >> FindOrBuildSolution(std::shared_ptr<GameTree> gt){
                        auto key = gt->StringDescription();

                        if( ! NoPersistent ){
                                auto iter = ss_.find(key);
                                if( iter != ss_.end()){
                                        if( iter->second.good() ){
                                                return iter->second.to_eigen_vv();
                                        } else {
                                                return {};
                                        }
                                }
                        }

                        auto ledger_key = ".ledger/" + key;
                        holdem_binary_strategy_ledger_s ledger;
                        if( ! NoPersistent ){
                                ledger.try_load_or_default(ledger_key);
                        }


                        GraphColouring<AggregateComputer> AG = MakeComputer(gt);
                        //auto sol = AlgebraicSolver(ledger, gt, AG);
                        auto solver = std::make_shared<CTNumericalSolver>();
                        auto sol = solver->ComputeSolution(gt, AG);

                        if( sol ){
                                ss_.add_solution(key, *sol);
                        } else {
                                // indicate bad solution
                                ss_.add_solution(key, holdem_binary_strategy_s{});
                        }

                        return sol; // might be nothing
                }
                void Display()const{
                        for(auto const& _ : ss_){
                                std::cout << "_.first => " << _.first << "\n"; // __CandyPrint__(cxx-print-scalar,_.first)
                        }
                }
        private:
                holdem_binary_solution_set_s ss_;
        };
        
        struct ScratchCmd : Command{
                enum{ Debug = 1};
                explicit
                ScratchCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        enum { Dp = 1 };
                        size_t n = 2;
                        double sb = 0.5;
                        double bb = 1.0;
                        #if 1
                        Driver dvr;
                        dvr.Display();
                        std::vector<Eigen::VectorXd> S;


                        std::shared_ptr<GameTree> any_gt;

                        std::vector<Pretty::LineItem> conv_tb;
                        conv_tb.push_back(std::vector<std::string>{"Desc", "?"});
                        conv_tb.push_back(Pretty::LineBreak);

                        //for(double eff = 2.0;eff - 1e-4 < 20.0; eff += 0.5 ){
                        for(double eff = 10.0;eff - 1e-4 < 10.0; eff += 0.5 ){
                                std::cout << "eff => " << eff << "\n"; // __CandyPrint__(cxx-print-scalar,eff)

                                std::shared_ptr<GameTree> gt;
                                #if 1
                                if( n == 2 ){
                                        gt = std::make_shared<GameTreeTwoPlayer>(sb, bb, eff);
                                } else {
                                        gt = std::make_shared<GameTreeThreePlayer>(sb, bb, eff);
                                }
                                #else
                                gt = std::make_shared<GameTreeTwoPlayerRaiseFold>(sb, bb, eff, 2.0);
                                #endif
                                any_gt = gt;
                                auto opt = dvr.FindOrBuildSolution(gt);
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

                        #endif
                        #if 0
                        auto opt = NumericalSolver(sb, bb, 10.0);
                        if( opt ){
                                pretty_print_strat(opt.get()[0][0], 0);
                                pretty_print_strat(opt.get()[1][0], 0);
                        }
                        #endif
                        Pretty::RenderTablePretty(std::cout, conv_tb);
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<ScratchCmd> ScratchCmdDecl{"scratch"};
} // end namespace ps
