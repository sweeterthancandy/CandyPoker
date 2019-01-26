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
                virtual void UpdateCandidateSolution(std::string const& uniq_key, StateType const& S)=0;
                virtual boost::optional<StateType> RetreiveCandidateSolution(std::string const& uniq_key)=0;

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
                        
                        std::string uniq_key = gt->StringDescription() + "::SimpleNumericalSolver";

                        StateType state0;
                        if(boost::optional<StateType> opt_state = ctx.RetreiveCandidateSolution(uniq_key)){
                                state0 = opt_state.get();
                        } else {
                                state0 = gt->MakeDefaultState();
                        }


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
                                ctx.UpdateCandidateSolution(uniq_key, S);
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

                        size_t Level;


                        friend bool operator<(Solution const& l, Solution const& r){ 
                                if( l.Level != r.Level ){
                                        return l.Level < r.Level;
                                } 
                                return l.Norm < r.Norm;
                        }

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

                                auto level = std::accumulate(gv.begin(), gv.end(), static_cast<size_t>(0) );

                                return {S, ev, S_counter, counter_ev, norm, mv, gv, level};
                        }
                };
                virtual void Execute(SolverContext& ctx)override{

                        std::string uniq_key = ctx.ArgGameTree()->StringDescription() + "::MinimalMixedSolutionSolver";

                        StateType state0;
                        if(boost::optional<StateType> opt_state = ctx.RetreiveCandidateSolution(uniq_key)){
                                state0 = opt_state.get();
                        } else {
                                state0 = ctx.ArgGameTree()->MakeDefaultState();
                        }

                        auto S = state0;

                        std::vector<Solution> ledger;
                        auto ret = NumericalPart_(ctx, uniq_key, ledger, S);
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
                                        ctx.UpdateCandidateSolution(uniq_key, S);
                                        BruteForcePart_(ctx, ledger);
                                        return;
                                }
                        }
                }
        private:
                ResultType NumericalPart_(SolverContext& ctx, std::string const& uniq_key, std::vector<Solution>& ledger, StateType& S){
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

                        
                        for(;Count!=MaxLoop;++Count){
                                for(size_t n=0;n!=LoopCount;++n){
                                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, Delta);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - Factor );
                                }
                                computation_kernel::InplaceClamp(S, ClampEpsilon);
                                ledger.push_back(Solution::MakeWithDeps(gt, AG, S));
                                ctx.UpdateCandidateSolution(uniq_key, S);

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
                        std::cout << "detail::to_string(ledger.back().Gamma) => " << detail::to_string(ledger.back().Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(sol.Gamma))
                        //return LoopOverFlow;
                        return FoundGamma;
                }
                struct MixedSolutionDescription{
                        MixedSolutionDescription(size_t player_index_, std::vector<size_t> const& mixed_)
                                :player_index(player_index_),
                                mixed(mixed_)
                        {}
                        friend std::ostream& operator<<(std::ostream& ostr, MixedSolutionDescription const& self){
                                ostr << "player_index = " << self.player_index;
                                typedef std::vector<size_t>::const_iterator CI0;
                                const char* comma = "";
                                ostr << "mixed" << " = {";
                                for(CI0 iter= self.mixed.begin(), end=self.mixed.end();iter!=end;++iter){
                                        ostr << comma << *iter;
                                        comma = ", ";
                                }
                                ostr << "}\n";
                                return ostr;
                        }
                        size_t player_index;
                        std::vector<size_t> mixed;
                };
                void BruteForcePart_(SolverContext& ctx, std::vector<Solution>& ledger){
                        double ClampEpsilon{1e-6};

                        auto gt = ctx.ArgGameTree();
                        auto AG = ctx.ArgComputer();

                        // find best one from the ledger
                                        
                        ctx.Message("Doing brute force search...");
                        
                        // pick any
                        #if 0
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
                        #endif
                        std::sort(ledger.begin(), ledger.end());
                        auto const& Sol = ledger.front();
                        auto const& S = Sol.S;
                        #if 0
                        for(auto _ : Sol.Gamma ){
                                if( 1 < _ ){
                                        std::cerr << "No ij9 gamma vector " << detail::to_string(Sol.Gamma) << "\n";
                                        return;
                                }
                        }
                        #endif

                        auto const& Counter = Sol.Counter;

                        
                        /*
                                Here we want to construct a vector which indicate all the mixed strategis.
                                For example if every was push/fold for hero/villian excep A2o for Hero, and 
                                67o and 79o for villian, we would have
                                                ({A2o}, {67o, 79o}).
                         */
                        std::vector<MixedSolutionDescription> mixed_info;
                        for(size_t idx=0;idx!=S.size();++idx){
                                auto const& c = Counter[idx][0];
                                auto const& t = S[idx][0];
                                std::vector<size_t> mixed;
                                for(size_t cid=0;cid!=169;++cid){
                                        #if 0
                                        if( t[cid] == 0.0 || t[cid] == 1.0 )
                                                continue;
                                        #endif
                                        if( t[cid] == c[cid] )
                                                continue;
                                        mixed.push_back(cid);
                                }
                                if( mixed.empty() )
                                        continue;
                                #if 0
                                if( mixed.size() == 1 ){
                                        mixed.push_back(mixed[0]-1);
                                        mixed.push_back(mixed[0]-1);
                                }
                                #endif
                                mixed_info.emplace_back(idx, std::move(mixed));
                        }
                        
                        for(auto const& mi : mixed_info){
                                std::cout << mi << "\n";
                        }

                        using factor_vector_type = std::vector<size_t>;
                        using factor_set_type = std::vector<factor_vector_type>;
                        using factor_family = std::vector<factor_set_type>;

                        std::vector<factor_family> family_vec;

                        enum{ GridSize = 10 };
                        
                        for(auto const& mi : mixed_info){
                                
                                factor_vector_type proto(mi.mixed.size(),1);

                                factor_family family;

                                family.push_back(factor_set_type{proto});

                                factor_set_type s;
                                for(size_t idx=0;idx!=mi.mixed.size();++idx){
                                        auto next = proto;
                                        next[idx] = GridSize;
                                        s.push_back(next);
                                }
                                family.push_back(s);

                                family_vec.push_back(std::move(family));

                        }

                        enum{ Debug = 1 };
                        if( Debug ){
                                std::stringstream sstr;
                                for(size_t idx=0;idx!=family_vec.size();++idx){
                                        if(idx != 0 )
                                                sstr << " x ";
                                        sstr << "{";
                                        for(size_t j=0;j!=family_vec[idx].size();++j){
                                                if(j != 0 )
                                                        sstr << ", ";
                                                sstr << "{";
                                                for(size_t k=0;k!=family_vec[idx][j].size();++k){
                                                        if(k != 0 )
                                                                sstr << ", ";
                                                        sstr << detail::to_string(family_vec[idx][j][k]);
                                                }
                                                sstr << "}";

                                        }
                                        sstr << "}";
                                }
                                std::cout << "sstr.str() => " << sstr.str() << "\n"; // __CandyPrint__(cxx-print-scalar,sstr.str())
                        }

                        size_t upper_bound = ( static_cast<size_t>(1) << mixed_info.size() );

                        std::vector<std::vector<factor_set_type> > cross_products;

                        enum{ MaxLevel = 1 };
                        for(size_t level = 0; level <= MaxLevel; ++level ){
                                for(size_t mask = 0; mask != upper_bound; ++mask ){
                                        if( __builtin_popcount(mask) != level )
                                                continue;
                                        cross_products.emplace_back();
                                        for(size_t idx=0;idx!=mixed_info.size();++idx){
                                                size_t cond = !! ( mask & static_cast<size_t>(1) << idx );
                                                cross_products.back().push_back( family_vec[idx][cond] );
                                        }
                                }
                        }


                        if( Debug ){
                                std::cout << "-------------------------------------\n";
                                for(auto const& cp : cross_products ){
                                        std::stringstream sstr;
                                        for(size_t idx=0;idx!=cp.size();++idx){
                                                if( idx != 0 ) sstr << " x ";
                                                sstr << "{";
                                                for(size_t k=0;k!=cp[idx].size();++k){
                                                        if(k != 0 )
                                                                sstr << ", ";
                                                        sstr << detail::to_string(cp[idx][k]);
                                                }
                                                sstr << "}";
                                        }
                                        std::cout << "sstr.str() => " << sstr.str() << "\n"; // __CandyPrint__(cxx-print-scalar,sstr.str())
                                }
                        }
                        
                        using realization_type = std::vector<factor_vector_type>;
                        using realizations_type = std::vector<realization_type>;
                        realizations_type realizations;
                        auto print_realizations = [&](){
                                std::cout << "-------------------------------------\n";
                                for(auto const& realization : realizations){
                                        std::stringstream sstr;
                                        for(size_t idx=0;idx!=realization.size();++idx){
                                                if( idx != 0 ) sstr << ", ";
                                                sstr << detail::to_string(realization[idx]);
                                        }
                                        std::cout << "sstr.str() => " << sstr.str() << "\n"; // __CandyPrint__(cxx-print-scalar,sstr.str())
                                }
                        };
                        print_realizations();
                        for(std::vector<factor_set_type> const& cp : cross_products){
                                realizations_type sub;
                                sub.emplace_back();
                                for(std::vector<factor_vector_type> const & group : cp ){
                                        auto proto = std::move(sub);
                                        for(factor_vector_type const& item : group ){
                                                auto next = proto;
                                                for(auto p : next){
                                                        p.push_back(item);
                                                        sub.push_back(p);
                                                }
                                        }
                                }
                                std::copy(sub.begin(), sub.end(), std::back_inserter(realizations));
                                print_realizations();
                        }

                        if( Debug ){
                                print_realizations();
                        }


                       std::vector<StateType> SV_family; 
                        for(auto const& realization : realizations){
                                std::vector<StateType> SV;
                                SV.push_back(S);
                                // for each player
                                for(size_t idx=0;idx!=mixed_info.size();++idx){
                                        auto const& mi = mixed_info[idx];
                                        size_t pid = mi.player_index;
                                        // for each mixed card
                                        for(size_t j=0;j!=mi.mixed.size();++j){


                                                // now N states increase N * (num_steps+1) states

                                                size_t cid = mi.mixed[j];
                                                size_t num_steps = realization[idx][j];
                                                auto proto = std::move(SV);
                                                for(size_t k=0;k<=num_steps;++k){
                                                        double pct = 1.0 / num_steps * k;
                                                        for(auto Q : proto ){
                                                                Q[pid][0][cid] =       pct;
                                                                Q[pid][1][cid] = 1.0 - pct;
                                                                SV.push_back(Q);
                                                        }
                                                }
                                        }
                                }
                                std::copy(SV.begin(), SV.end(), std::back_inserter(SV_family));
                        }

                        ctx.Message("Doing last bit");

                        std::vector<Solution> solution_candidates;

                        std::cout << "SV_family.size() => " << SV_family.size() << "\n"; // __CandyPrint__(cxx-print-scalar,SV_family.size())
                        boost::optional<Solution> best;

                        for(size_t idx=0;idx!=SV_family.size();++idx){
                                std::cout << "idx => " << idx << "\n"; // __CandyPrint__(cxx-print-scalar,idx)
                                auto Q = SV_family[idx];
                                computation_kernel::InplaceClamp(Q, ClampEpsilon);
                                auto candidate = Solution::MakeWithDeps(gt, AG, Q);
                                solution_candidates.push_back(candidate);
                        }

                        std::vector<Pretty::LineItem> dbg;
                        dbg.push_back(std::vector<std::string>{"n", "|.|", "Gamma", "Mixed"});
                        dbg.push_back(Pretty::LineBreak);

                        std::sort(solution_candidates.begin(), solution_candidates.end());

                        for(auto const& sol : solution_candidates){

                                std::vector<std::string> dbg_line;
                                dbg_line.push_back(boost::lexical_cast<std::string>(sol.Level));
                                dbg_line.push_back(boost::lexical_cast<std::string>(sol.Norm));
                                dbg_line.push_back(detail::to_string(sol.Gamma));
                                dbg_line.push_back(detail::to_string(sol.Mixed));
                                dbg.push_back(std::move(dbg_line));
                        }


                        Pretty::RenderTablePretty(std::cout, dbg);

                        if( solution_candidates.size() && solution_candidates.front().Level <= 1 ){
                                auto const& best = solution_candidates.front();
                                std::cout << "detail::to_string(best.Gamma) => " << detail::to_string(best.Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(best.Gamma))
                                ctx.EmitSolution(best.S);
                        } else {
                                ctx.Message("no best :(");
                        }
                        
                }
        };









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
                                {
                                        ss_.try_load_or_default(".ps.context.ss");
                                }

                                virtual std::shared_ptr<GameTree>         ArgGameTree()override{ return gt_; }
                                virtual GraphColouring<AggregateComputer> ArgComputer()override{ return AG_; }
                                //virtual boost::property_tree::ptree       ArgExtra()=0;

                                // A large part of the solution is being able to run it for 6 hours,
                                // then turn off the computer, and come back to it at a later date
                                virtual void Message(std::string const& msg){
                                        std::cerr << msg << "\n";
                                }
                                virtual void UpdateCandidateSolution(std::string const& uniq_key, StateType const& S){
                                        enum{ Dp = 10};
                                        DisplayStrategy(S, Dp);

                                        ss_.add_solution(uniq_key, S);
                                        ss_.save_();

                                }
                                virtual boost::optional<StateType> RetreiveCandidateSolution(std::string const& uniq_key){
                                        auto iter = ss_.find(uniq_key);
                                        if( iter != ss_.end()){
                                                return iter->second.to_eigen_vv();
                                        }
                                        return {};
                                }

                                virtual void EmitSolution(StateType const& S){
                                        BOOST_ASSERT( ! S_ );
                                        S_ = S;
                                }
                                auto const& Get()const{ return S_.get(); }
                        private:
                                std::shared_ptr<GameTree> gt_;
                                GraphColouring<AggregateComputer> AG_;
                                boost::optional<StateType> S_;
                                holdem_binary_solution_set_s ss_;
                        };
                        Context ctx{gt, AG};
                        //SimpleNumericalSolver{}.Execute(ctx);

                        MinimalMixedSolutionSolver{}.Execute(ctx);
                        return ctx.Get();

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

                        //for(double eff = 2.0;eff - 1e-4 < 20.0; eff += 0.1 ){
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
