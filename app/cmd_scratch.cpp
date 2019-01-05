#include "ps/base/cards.h"
#include "ps/support/command.h"
#include "ps/detail/print.h"
#include "ps/eval/class_cache.h"
#include "ps/eval/holdem_class_vector_cache.h"
#include "app/pretty_printer.h"
#include "app/serialization_util.h"
#include "ps/detail/graph.h"
#include "ps/support/any_context.h"

#include "ps/sim/computer.h"
#include "ps/sim/game_tree.h"
#include <boost/any.hpp>

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
                 * Because it's expensive for calculate
                 * the Counter strategy, we keep it here on
                 * the object rather than do the calculation
                 * multiple times
                 */
                StateType Counter;
                Eigen::VectorXd Counter_EV;

                double Norm;

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


                static Solution MakeWithDeps(std::shared_ptr<GameTree> gt,
                                             GraphColouring<AggregateComputer>& AG,
                                             StateType const& S)
                {
                        auto root = gt->Root();
                        auto ev = AG.Color(root).ExpectedValue(S);

                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, 0.0);
                        auto counter_ev = AG.Color(root).ExpectedValue(S_counter);

                        auto d = ev - counter_ev;

                        auto norm = d.lpNorm<Eigen::Infinity>();

                        auto mv = computation_kernel::MixedVector( gt, S );
                        auto gv = computation_kernel::GammaVector( gt, AG, S );

                        return {S, ev, S_counter, counter_ev, norm, mv, gv};
                }
        };

        






        void DisplayStrategy(StateType const& S, size_t dp = 4){
                for(size_t idx=0;idx!=S.size();++idx){
                        pretty_print_strat(S[idx][0], dp);
                }
        }



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



        struct Context{
                std::shared_ptr<GameTree> gt;
                GraphColouring<AggregateComputer>& AG;

                StateType S;
                GNode* root;
                double ClampEpsilon{1e-6};


                std::vector<Solution> Ledger;

                size_t Count{0};
                size_t MaxCount{100};


                void DoDisplay()const{
                        auto const& sol = Ledger.back();
                        std::cout << "\n==================================================================================\n\n";
                        std::cout << "Count = " << Count << "\n";
                        std::cout << "MaxCount = " << MaxCount << "\n";
                        std::cout << "sol.Norm = " << sol.Norm << "\n";
                        DisplayStrategy(sol.S, 8);
                        std::vector<std::string> header{"N", "EV", "Norm", "Mixed", "Gamma"};
                        using namespace Pretty;
                        std::vector<LineItem> buf{header, LineBreak};
                        for(size_t idx=0;idx!=Ledger.size();++idx){
                                auto const& L = Ledger[idx];
                                std::vector<std::string> line; 
                                line.push_back(boost::lexical_cast<std::string>(idx));
                                line.push_back(vector_to_string(L.EV));
                                line.push_back(boost::lexical_cast<std::string>(L.Norm));
                                line.push_back(detail::to_string(L.Mixed));
                                line.push_back(detail::to_string(L.Gamma));
                                buf.push_back(line);
                        }
                        RenderTablePretty(std::cout, buf);
                }
        };




        struct StaticLoop{
                size_t LoopCount{10};
                double Factor{0.05};

                enum ResultType{
                        FoundGamma,
                        FoundPerfect,
                        LoopOverFlow,
                };

                ResultType operator()(Context* in){
                        enum{ MaxLoop = 100 };
                        size_t Count=0;
                        double Epsilon{1e-4};
                        for(;Count!=MaxLoop;++Count){
                                for(size_t n=0;n!=LoopCount;++n){
                                        auto S_counter = computation_kernel::CounterStrategy(in->gt, in->AG, in->S, 0.0);
                                        computation_kernel::InplaceLinearCombination(in->S, S_counter, 1 - Factor );
                                }
                                computation_kernel::InplaceClamp(in->S, in->ClampEpsilon);
                                in->Ledger.push_back(Solution::MakeWithDeps(in->gt, in->AG, in->S));
                                
                                auto& sol = in->Ledger.back();

                                #if 0
                                if( computation_kernel::IsMinMixedSolution( sol.Mixed ) ){
                                        auto dp = ctrl->DeclPath();
                                        dp->Next(std::make_shared<GridSolver>());
                                        ctrl->Pass();
                                }
                                #endif
                                if( sol.Norm < Epsilon ){
                                        Epsilon /= 2.0;
                                        Count = 0;
                                }
                                
                                if( in->Ledger.back().Gamma == std::vector<size_t>{0,0} ){
                                        return FoundPerfect;
                                }
                                
                                if( in->Ledger.back().Gamma == std::vector<size_t>{1,1} ||
                                    in->Ledger.back().Gamma == std::vector<size_t>{1,0} ||
                                    in->Ledger.back().Gamma == std::vector<size_t>{0,1} ){
                                        return FoundGamma;
                                }

                                in->DoDisplay();
                        }
                        return LoopOverFlow;
                }
        };
        
        /*
         * The idea here is that the solution we get numerically won't converge to a satisfactory 
         * solition, mainly because without a forcing it will be heavily mixed, and also it would
         * never be balanced. 
         */
        struct GridSolver{
                enum ResultType{
                        Success,
                        Error,
                };
                ResultType operator()(Context* in)
                {
                        // find best one from the ledger
                        
                        // pick any
                        std::vector<Solution const*> candidates;
                        for(auto const& S : in->Ledger){
                                if( computation_kernel::IsMinMixedSolution( S.Mixed ) ){
                                        candidates.push_back(&S);
                                }
                        }
                        std::cout << "candidates.size() = " << candidates.size() << "\n";
                        std::sort( candidates.begin(), candidates.end(),
                                   [](auto l, auto r)
                                   {
                                        auto la = std::accumulate(l->Mixed.begin(), l->Mixed.end(), 0 );
                                        auto ra = std::accumulate(r->Mixed.begin(), r->Mixed.end(), 0 );
                                        if( la != ra )
                                                return la < ra;

                                        return l->Norm < r->Norm;
                                   }
                        );
                        
                        auto const& Sol = *candidates.front();
                        auto const& S = Sol.S;
                        for(auto _ : Sol.Gamma ){
                                if( 1 != _ ){
                                        std::cerr << "No [1,1] gamma vector " << detail::to_string(Sol.Gamma) << "\n";
                                        return Error;
                                }
                        }
                        std::vector<std::vector<size_t> > CIDS(S.size());
                        for(size_t idx=0;idx!=S.size();++idx){
                                auto const& t = S[idx][0];
                                for(size_t cid=0;cid!=169;++cid){
                                        if( t[cid] == 0.0 || t[cid] == 1.0 )
                                                continue;
                                        CIDS[idx].push_back(cid);
                                }
                        }

                        std::cout << "detail::to_string(CIDS[0]) = " << detail::to_string(CIDS[0]) << "\n";
                        std::cout << "detail::to_string(CIDS[1]) = " << detail::to_string(CIDS[1]) << "\n";

                        boost::optional<Solution> best;

                        auto try_solution = [&](double a, double b){
                                auto Q = S;
                                Q[0][0][CIDS[0][0]] = a;
                                Q[0][1][CIDS[0][0]] = 1.0 - Q[0][0][CIDS[0][0]];
                                Q[1][0][CIDS[1][0]] = b;
                                Q[1][1][CIDS[1][0]] = 1.0 - Q[1][0][CIDS[1][0]];
                                computation_kernel::InplaceClamp(Q, in->ClampEpsilon);
                                auto candidate = Solution::MakeWithDeps(in->gt, in->AG, Q);
                                #if 0
                                std::cout << "detail::to_string(candidate.Gamma) = " << detail::to_string(candidate.Gamma) << "\n";
                                std::cout << "candidate.Norm = " << std::fixed << std::setprecision(30) << candidate.Norm << "\n";
                                #endif
                                if( candidate.Gamma != std::vector<size_t>{1,1} &&
                                    candidate.Gamma != std::vector<size_t>{0,1} &&
                                    candidate.Gamma != std::vector<size_t>{1,0} ){
                                        return;
                                }
                                if( ! best ){
                                        best = std::move(candidate);
                                        return;
                                }
                                if( candidate.Norm < best->Norm ){
                                        std::cout << "candidate.Norm = " << candidate.Norm << "\n";
                                        best = std::move(candidate);
                                        return;
                                }
                        };

                        
                        // \foreach val \in [0,100]
                        for(size_t val=0;val<=100;++val){
                                //          ^ including
                                try_solution( val /100.0, 0 );
                                try_solution( val /100.0, 1 );
                                try_solution( 0, val /100.0);
                                try_solution( 1, val /100.0);
                        }
                        if( best ){
                                in->Ledger.push_back(best.get());
                        }


                        return Success;
                }
        };

        





        
        
        struct CTNumericalSolver : SolverConcept{
                virtual boost::optional<StateType> ComputeSolution(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer>& AG)const noexcept override
                {
                        
                        auto pp = std::make_shared<Context>(Context{gt, AG});
                        pp->S = gt->MakeDefaultState();
                        pp->root = gt->Root();
                        std::vector<Solution> ledger;

                        switch(StaticLoop{}(pp.get()))
                        {
                                case StaticLoop::LoopOverFlow:
                                {
                                        BOOST_THROW_EXCEPTION(std::domain_error("not convergence"));
                                }
                                case StaticLoop::FoundPerfect:
                                {
                                        break;
                                }
                                case StaticLoop::FoundGamma:
                                {
                                        if(GridSolver{}(pp.get()) != GridSolver::Success)
                                                BOOST_THROW_EXCEPTION(std::domain_error("not convergence in grid"));
                                        break;
                                }
                        }


                        #if 0
                        std::vector<Pretty::LineItem> buf;
                        for(size_t idx=0;idx!=pp->Ledger.size();++idx){
                                auto const& sol = pp->Ledger[idx];
                                std::vector<std::string> line;
                                line.push_back(boost::lexical_cast<std::string>(idx));
                                line.push_back(vector_to_string(sol.EV));
                                line.push_back(boost::lexical_cast<std::string>(sol.Norm));

                                buf.push_back(line);
                        }
                        Pretty::RenderTablePretty(std::cout, buf);

                        std::cout << "pp->Ledger.size() = " << pp->Ledger.size() << "\n";
                        #endif
                        return pp->Ledger.back().S;
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

                        for(double eff = 2.0;eff - 1e-4 < 20.0; eff += 1.0 ){
                        //for(double eff = 10.0;eff - 1e-4 < 10.0; eff += 0.5 ){
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
