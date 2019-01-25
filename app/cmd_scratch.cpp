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
        
        void DisplayStrategy(StateType const& S, size_t dp = 4){
                for(size_t idx=0;idx!=S.size();++idx){
                        pretty_print_strat(S[idx][0], dp);
                }
        }

        /*
                After several iterations, I 
         */
        struct SolverContext{
                virtual ~SolverContext()=default;

                // ultimatley the input
                virtual std::shared_ptr<GameTree>         ArgGameTree()=0;
                virtual GraphColouring<AggregateComputer> ArgComputer()=0;
                //virtual boost::property_tree::ptree       ArgExtra()=0;

                // A large part of the solution is being able to run it for 6 hours,
                // then turn off the computer, and come back to it at a later date
                virtual void Message(std::string const& msg)=0;
                virtual void UpdateCandidateSolution(StateType const& S)=0;
                virtual boost::optional<StateType> RetreiveCandidateSolution()=0;

                virtual void EmitSolution(StateType const& S)=0;

        };

        struct Solver{
                virtual ~Solver()=default;
                virtual void Execute(SolverContext& ctx)=0;
        };

        struct SimpleNumericalSolver : Solver{
                virtual void Execute(SolverContext& ctx)override{
                        double factor_ = 0.05;
                        double epsilon = 0.00001;
                        enum{ Stride = 10 };
                        auto gt = ctx.ArgGameTree();
                        auto root   = gt->Root();
                        auto state0 = gt->MakeDefaultState();

                        auto AG   = ctx.ArgComputer();
                        
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
                                ctx.UpdateCandidateSolution(S);
                                if( norm < epsilon ){
                                        ctx.EmitSolution(S);
                                        return;
                                }
                        }
                }
        };

        struct MinimalMixedSolutionSolver : Solver{
                enum ResultType{
                        FoundGamma,
                        FoundPerfect,
                        LoopOverFlow,
                };
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
                virtual void Execute(SolverContext& ctx)override{

                        auto S = ctx.ArgGameTree()->MakeDefaultState();

                        std::vector<Solution> ledger;
                        auto ret = NumericalPart_(ctx, ledger, S);
                        switch(ret)
                        {
                                case LoopOverFlow:
                                {
                                        BOOST_THROW_EXCEPTION(std::domain_error("not convergence"));
                                }
                                case FoundPerfect:
                                {
                                        ctx.EmitSolution(S);
                                        return;
                                }
                                case FoundGamma:
                                {
                                        ctx.UpdateCandidateSolution(S);
                                        BruteForcePart_(ctx, ledger);
                                        return;
                                }
                        }
                }
        private:
                ResultType NumericalPart_(SolverContext& ctx, std::vector<Solution>& ledger, StateType& S){
                        size_t LoopCount{10};
                        double Factor{0.05};
                        enum{ MaxLoop = 100 };
                        size_t Count=0;
                        double Epsilon{1e-4};
                        double ClampEpsilon{1e-6};

                        double Delta = 0.0;

                        auto gt = ctx.ArgGameTree();
                        auto AG = ctx.ArgComputer();
                        
                        auto root = gt->Root();

                        
                        for(size_t Outer=0;Outer!=3;++Outer){
                                for(;Count!=MaxLoop;++Count){
                                        for(size_t n=0;n!=LoopCount;++n){
                                                auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, Delta);
                                                computation_kernel::InplaceLinearCombination(S, S_counter, 1 - Factor );
                                        }
                                        computation_kernel::InplaceClamp(S, ClampEpsilon);
                                        ledger.push_back(Solution::MakeWithDeps(gt, AG, S));
                                        ctx.UpdateCandidateSolution(S);

                                        auto& sol = ledger.back();

                                        if( sol.Norm < Epsilon ){
                                                Epsilon /= 2.0;
                                                Count = 0;
                                        }
                                        
                                        if( sol.Gamma == std::vector<size_t>{0,0} ){
                                                std::cout << "detail::to_string(sol.Gamma) => " << detail::to_string(sol.Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(sol.Gamma))
                                                return FoundPerfect;
                                        }
                                        
                                        if( sol.Gamma == std::vector<size_t>{1,1} ||
                                            sol.Gamma == std::vector<size_t>{1,0} ||
                                            sol.Gamma == std::vector<size_t>{0,1} )
                                        {
                                                std::cout << "detail::to_string(sol.Gamma) => " << detail::to_string(sol.Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(sol.Gamma))
                                                return FoundGamma;
                                        }
                                }
                                Count = 0;
                                Delta += 0.005;
                        }
                        return LoopOverFlow;
                }
                void BruteForcePart_(SolverContext& ctx, std::vector<Solution> const& ledger){
                        double ClampEpsilon{1e-6};

                        auto gt = ctx.ArgGameTree();
                        auto AG = ctx.ArgComputer();

                        // find best one from the ledger
                                        
                        ctx.Message("Doing brute force search...");
                        
                        // pick any
                        std::vector<Solution const*> candidates;
                        for(auto const& S : ledger){
                                if( computation_kernel::IsMinMixedSolution( S.Mixed ) ){
                                        candidates.push_back(&S);
                                }
                        }
                        std::stringstream msg;
                        msg << "Got " << candidates.size() << " candidates";
                        ctx.Message(msg.str());

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
                                if( 1 < _ ){
                                        std::cerr << "No ij9 gamma vector " << detail::to_string(Sol.Gamma) << "\n";
                                        return;
                                }
                        }

                        auto const& Counter = Sol.Counter;
                        
                        /*
                                Here we want to construct a vector which indicate all the mixed strategis.
                                For example if every was push/fold for hero/villian excep A2o for Hero, and 
                                67o and 79o for villian, we would have
                                                ({A2o}, {67o, 79o}).
                         */
                        std::vector<std::vector<size_t> > CIDS(S.size());
                        for(size_t idx=0;idx!=S.size();++idx){
                                auto const& c = Counter[idx][0];
                                auto const& t = S[idx][0];
                                for(size_t cid=0;cid!=169;++cid){
                                        #if 0
                                        if( t[cid] == 0.0 || t[cid] == 1.0 )
                                                continue;
                                        #endif
                                        if( t[cid] == c[cid] )
                                                continue;
                                        CIDS[idx].push_back(cid);
                                }
                        }

                        std::cout << "detail::to_string(CIDS[0]) = " << detail::to_string(CIDS[0]) << "\n";
                        std::cout << "detail::to_string(CIDS[1]) = " << detail::to_string(CIDS[1]) << "\n";

                        boost::optional<Solution> best;

                        std::vector<StateType> cand_vec;
                        auto consume_candidate = [&](auto Q){
                                cand_vec.push_back(Q);
                        };

                
                        ctx.Message("Doing brute force bit");
                        for(size_t idx=0;idx!=CIDS.size();++idx){
                                if( CIDS[idx].empty())
                                        continue;
                                std::vector<StateType> SV;
                                SV.push_back(S);
                                for(size_t j=0;j!=CIDS.size();++j){
                                        if( j == idx)
                                                continue;
                                        if( CIDS[j].empty())
                                                continue;
                                        auto gen = std::move(SV);
                                        SV.clear();
                                        for(auto const& s : gen){
                                                SV.push_back(s);
                                                SV.back()[j][0][CIDS[j][0]] = 0;
                                                SV.back()[j][1][CIDS[j][0]] = 1;
                                                SV.push_back(s);
                                                SV.back()[j][1][CIDS[j][0]] = 1;
                                                SV.back()[j][0][CIDS[j][0]] = 0;
                                        }
                                }
                                enum{ GridSteps = 10 };
                                for(size_t k=0;k<=GridSteps;++k){
                                        double pct = 1.0 / GridSteps * k;
                                        std::cout << "pct => " << pct << "\n"; // __CandyPrint__(cxx-print-scalar,pct)
                                        for(auto& s : SV){
                                                s[idx][0][CIDS[idx][0]] = pct;
                                                s[idx][1][CIDS[idx][0]] = 1.0 - pct;

                                                consume_candidate(s);
                                        }
                                }
                        }

                        ctx.Message("Doing last bit");
                        std::cout << "cand_vec.size() = " << cand_vec.size() << "\n";
                        for(auto& Q : cand_vec){
                                computation_kernel::InplaceClamp(Q, ClampEpsilon);
                                auto candidate = Solution::MakeWithDeps(gt, AG, Q);
                                #if 0
                                std::cout << "detail::to_string(candidate.Gamma) = " << detail::to_string(candidate.Gamma) << "\n";
                                std::cout << "candidate.Norm = " << std::fixed << std::setprecision(30) << candidate.Norm << "\n";
                                #endif

                                if( candidate.Gamma != std::vector<size_t>{1,1} &&
                                    candidate.Gamma != std::vector<size_t>{0,1} &&
                                    candidate.Gamma != std::vector<size_t>{0,0} &&
                                    candidate.Gamma != std::vector<size_t>{1,0} ){
                                        continue;
                                }
                                if( ! best ){
                                        best = std::move(candidate);
                                        continue;
                                }
                                if( candidate.Norm < best->Norm ){
                                        std::cout << "candidate.Norm = " << candidate.Norm << "\n";
                                        best = std::move(candidate);
                                        continue;
                                }
                        }


                        #if 0
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
                        #endif


                        if( best ){
                                std::cout << "detail::to_string(best.get().Gamma) => " << detail::to_string(best.get().Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(best.get().Gamma))
                                ctx.EmitSolution(best.get().S);
                        } else {
                                ctx.Message("no best :(");
                        }
                        for(auto& Q : cand_vec){
                                computation_kernel::InplaceClamp(Q, ClampEpsilon);
                                auto candidate = Solution::MakeWithDeps(gt, AG, Q);


                                std::cout << "detail::to_string(candidate.Gamma) => " << detail::to_string(candidate.Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(candidate.Gamma))
                        }
                }
        };


        #if 0
        struct SolutionBase{
                SolutionBase(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer>& AG, StateType const& S)
                        : gt_{gt}
                        , AG_{&AG}
                        , S_{S}
                }
        protected:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer>* AG_;
                StateType const& S_;
        };

        template<class... Attrs>
        struct GeneralizedSolution
                : SolutionBase
                , Attrs::template Build<GeneralizedSolution<Attrs...> >...
        {
                GeneralizedSolution(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer>& AG, StateType const& S)
                        : SolutionBase{gt, AG, S}
                {}
        };

        struct GammaAttribute{
                template<class Self>
                struct Build{
                        std::vector<size_t> const& GammaVector()const{
                                auto* self = dynamic_cast<Self const*>(this);
                                if( vec_.empty() ){
                                        vec_ = computation_kernel::GammaVector( self->gt_, *(self->AG_), self->S_ );
                                }
                        }
                private:
                        mutable std::vector<size_t> vec_;
                };
        };
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
                        //auto solver = std::make_shared<CTNumericalSolver>();



                        #if 0
                        auto sol = solver->ComputeSolution(gt, AG);
                        if( sol ){
                                ss_.add_solution(key, *sol);
                        } else {
                                // indicate bad solution
                                ss_.add_solution(key, holdem_binary_strategy_s{});
                        }

                        return sol; // might be nothing
                        #endif
                        struct Context : SolverContext{
                                Context(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG)
                                        :gt_(gt),
                                        AG_(AG)
                                {}
                                std::shared_ptr<GameTree> gt_;
                                GraphColouring<AggregateComputer> AG_;
                                boost::optional<StateType> S_;

                                virtual std::shared_ptr<GameTree>         ArgGameTree()override{ return gt_; }
                                virtual GraphColouring<AggregateComputer> ArgComputer()override{ return AG_; }
                                //virtual boost::property_tree::ptree       ArgExtra()=0;

                                // A large part of the solution is being able to run it for 6 hours,
                                // then turn off the computer, and come back to it at a later date
                                virtual void Message(std::string const& msg){
                                        std::cerr << msg << "\n";
                                }
                                virtual void UpdateCandidateSolution(StateType const& S){
                                        enum{ Dp = 10};
                                        DisplayStrategy(S, Dp);
                                }
                                virtual boost::optional<StateType> RetreiveCandidateSolution(){ return {}; }

                                virtual void EmitSolution(StateType const& S){
                                        BOOST_ASSERT( ! S_ );
                                        S_ = S;
                                }
                        };
                        Context ctx{gt, AG};
                        //SimpleNumericalSolver{}.Execute(ctx);

                        MinimalMixedSolutionSolver{}.Execute(ctx);
                        return ctx.S_;

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

                        for(double eff = 5.0;eff - 1e-4 < 20.0; eff += 1.0 ){
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
                                std::exit(0);
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
