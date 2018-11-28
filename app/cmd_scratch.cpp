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
                           



#include <boost/timer/timer.hpp>

namespace ps{
        using namespace sim;
        



        template<class Value, class Error_>
        struct BasicValueOrError{
                BasicValueOrError(Error_ const& err):
                        error_{err}
                {}
                BasicValueOrError(Value const& val):
                        value_{val}
                {}
                bool IsError()const{ return value_.operator bool(); }
                operator bool()const{ return IsError(); }
                auto AsError()const{
                        if( ! IsError() )
                                BOOST_THROW_EXCEPTION(std::domain_error("is not an error"));
                        return error_.get();
                }
                Value const& get()const{
                        if( IsError() )
                                BOOST_THROW_EXCEPTION(std::domain_error("is an error"));
                        return value_.get();
                }
        private:
                boost::optional<Value> value_;
                boost::optional<Error_> error_;
        };

        struct Error{
                std::string msg;
        };
        template<class T>
        using ValueOrError = BasicValueOrError<T, Error>;
        
        StateType& InplaceLinearCombination(StateType& x,
                                            StateType const& y,
                                            double alpha)
        {
                for(size_t i=0;i!=x.size();++i){
                        for(size_t j=0;j!=x[i].size();++j){
                                x[i][j] *= alpha;
                                x[i][j] += y[i][j] * ( 1.0 - alpha );
                        }
                }
                return x;
        }
        StateType& InplaceClamp(StateType& x, double epsilon)
        {
                for(size_t i=0;i!=x.size();++i){
                        for(size_t j=0;j!=x[i].size();++j){
                                for(size_t cid=0;cid!=169;++cid){
                                        if( std::fabs( x[i][j][cid] ) < epsilon ){
                                                x[i][j][cid] = 0.0;
                                        }
                                        if( std::fabs( 1.0 - x[i][j][cid] ) < epsilon ){
                                                x[i][j][cid] = 1.0;
                                        }
                                }
                        }
                }
                return x;
        }



        struct GeometricLoopOptions{
                size_t MaxIter{1000};
                double Delta{0.01};
                size_t Stride{1};
                double Factor{0.05};
                double Epsilon{0.001};
        };
        struct Solution{
                StateType S;
                boost::any Category;
        };
        struct AnyObserver{
                virtual ~AnyObserver()=default;
                virtual boost::any Condition(StateType const& S){ return {}; };
                virtual void Start(StateType const& S){ }
                virtual void Finish(StateType const& S){ }
        };
        Solution GeometricLoopWithClamp(GeometricLoopOptions const& opts,
                                        std::shared_ptr<GameTree> gt,
                                        GraphColouring<AggregateComputer> const& AG,
                                        StateType const& S0,
                                        std::vector<std::shared_ptr<AnyObserver> > const& obs)
        {
                size_t counter = 0;
                auto S = S0;

                for(auto& _ : obs){
                        _->Start(S0);
                }

                for(;counter < opts.MaxIter;){
                        for(size_t inner=0;inner!=opts.Stride;++inner, ++counter){
                                auto S_counter = CounterStrategy(gt, AG, S, opts.Delta);
                                InplaceLinearCombination(S, S_counter, 1 - opts.Factor );
                        }

                        InplaceClamp(S, opts.Epsilon);

                        boost::any cond;
                        for(auto& _ : obs){
                                cond = _->Condition(S);
                                if( ! cond.empty()){
                                        break;
                                }
                        }

                        if( ! cond.empty()){
                                for(auto& _ : obs){
                                        _->Finish(S);
                                }
                                return Solution{ S, cond };
                        }
                }
                return {};
        }

        std::vector<size_t> GammaVector( std::shared_ptr<GameTree> gt,
                                         GraphColouring<AggregateComputer> const& AG,
                                         StateType const& S)
        {
                // we have a mixed solution where the counter strategy 
                // has only one cid different from our solutuon.
                auto counter_nf = CounterStrategy(gt, AG, S, 0.0);
                std::vector<size_t> gamma_vec;
                for(size_t idx=0;idx!=S.size();++idx){
                        auto A = S[idx][0] - counter_nf[idx][0];
                        gamma_vec.push_back(0);
                        for(size_t idx=0;idx!=169;++idx){
                                if( A[idx] != 0.0 ){
                                        ++gamma_vec.back();
                                }
                        }
                }
                return gamma_vec;
        }
        bool IsMinMixedSolution(std::vector<size_t> const& gamma_vec){
                enum{
                        T_Zero        = 0,
                        T_One         = 1,
                        T_Two         = 2,
                        T_ThreeOrMore = 3,
                };
                std::array<size_t, 4> M = {0, 0, 0, 0 };
                for(size_t idx=0;idx!=gamma_vec.size();++idx){
                        auto j = std::min<size_t>(gamma_vec[idx], 3);
                        ++M[j];
                }
                if( M[T_ThreeOrMore] == 0 && M[T_Two] <= 1 ){
                        return true;
                }
                return false;
        }

        struct Printer{
                Printer(size_t n):n_{n}{
                        std::vector<std::string> title;
                        title.push_back("n");
                        for(size_t idx=0;idx!=n_;++idx){
                                std::stringstream sstr;
                                sstr << "EV[" << idx << "]";
                                title.push_back(sstr.str());
                        }
                        for(size_t idx=0;idx!=n_;++idx){
                                std::stringstream sstr;
                                sstr << "CEV[" << idx << "]";
                                title.push_back(sstr.str());
                        }
                        title.push_back("|.|");
                        lines_.emplace_back(std::move(title));
                        lines_.emplace_back(Pretty::LineBreak);
                }
                void operator()(std::vector<std::vector<Eigen::VectorXd> > const& S,
                                Eigen::VectorXd const& ev,
                                Eigen::VectorXd const& ev_counter,
                                double norm){
                        std::vector<std::string> l;
                        l.push_back(boost::lexical_cast<std::string>(count_++));
                        for(size_t idx=0;idx!=n_;++idx){
                                l.push_back(boost::lexical_cast<std::string>(ev[idx]));
                        }
                        for(size_t idx=0;idx!=n_;++idx){
                                l.push_back(boost::lexical_cast<std::string>(ev_counter[idx]));
                        }
                        l.push_back(boost::lexical_cast<std::string>(norm));
                        size_t zero_or_one = 0;
                        size_t total = 0;
                        for(size_t idx=0;idx!=169;++idx){
                                double epsilon = 1e-5;
                                for(size_t j=0;j!=S.size();++j, ++total){
                                        if( S[j][0][idx] < epsilon || 1 - S[j][0][idx] < epsilon ){
                                                ++zero_or_one;
                                        }
                                }
                        }
                        l.push_back(boost::lexical_cast<std::string>(zero_or_one));
                        l.push_back(boost::lexical_cast<std::string>(total));
                        l.push_back(boost::lexical_cast<std::string>(S[0][0].lpNorm<Eigen::Infinity>()));

                        lines_.emplace_back(std::move(l));
                }
                void Display()const{

                        Pretty::RenderOptions opts;
                        opts.SetAdjustment(1, Pretty::RenderAdjustment_Left);
                        opts.SetAdjustment(2, Pretty::RenderAdjustment_Left);
                        opts.SetAdjustment(3, Pretty::RenderAdjustment_Left);

                        Pretty::RenderTablePretty(std::cout, lines_, opts);
                }
        private:
                size_t n_;
                size_t count_{0};
                std::vector<Pretty::LineItem> lines_;
        };

        GraphColouring<AggregateComputer> MakeComputer(std::shared_ptr<GameTree> gt){
                auto root   = gt->Root();
                auto state0 = gt->InitialState();



                
                /*
                 * To simplify the construction, we first create a object which represents
                 * the teriminal state of the game, for hu we have 3 terminal states
                 *                        {f,pf,pp},
                 * with the first two being independent of the dealt hand, whilst the 
                 * last state pp required all in equity evaluation. We create a vector
                 * of each of these terminal states, then which create Computer objects
                 * for fast computation. This is mainly to simplify the code.
                 */
                std::vector<std::shared_ptr<MakerConcept> > maker_dev;

                /*
                 * Now we go over all the terminal nodes, which is basically a path to
                 * something that can be evaluated. 
                 */
                GraphColouring<AggregateComputer> AG;
                GraphColouring<PushFoldOperator> ops;
                gt->ColorOperators(ops);

                for(auto t : root->TerminalNodes() ){

                        auto path = t->EdgePath();
                        auto ptr = std::make_shared<Computer>(path_to_string(path));
                        AG[t].push_back(ptr);
                        for(auto e : path ){
                                AG[e->From()].push_back(ptr); 
                        }

                        /*
                         * Work out the terminal state, independent of the deal, and
                         * make an factory object
                         */
                        auto state = state0;
                        for( auto e : path ){
                                ops[e](state);
                        }
                        std::vector<size_t> perm(state.Active.begin(), state.Active.end());
                        maker_dev.push_back(std::make_shared<StaticEmit>(ptr, path, perm, state.Pot) );

                }
                
                std::string cache_name{".cc.bin.prod"};
                class_cache C;
                C.load(cache_name);
                IndexMaker im(*gt);
                        
                MakerConcept::Display(maker_dev);

                auto nv = [&](double prob, holdem_class_vector const& cv){
                        for(auto& m : maker_dev ){
                                m->Emit(&im, &C, prob, cv );
                        }
                };
                gt->VisitProbabilityDist(nv);

                return AG;
        }

        struct NonMixedSolutionSolution{
                size_t metric;
        };
        struct NonMixedSolutionSolutionCondition : AnyObserver{
                enum{ Debug = true };
                NonMixedSolutionSolutionCondition(std::shared_ptr<GameTree> gt,
                                   GraphColouring<AggregateComputer> const& AG)
                        : gt_{gt}, AG_{&AG}
                {}
                boost::any Condition(StateType const& S)override{
                        for(size_t idx=0;idx!=S.size();++idx){
                                for(size_t cid=0;cid!=169;++cid){
                                        if( S[idx][0][cid] == 0.0 || S[idx][0][cid] == 1.0 )
                                                continue;
                                        return {};
                                }
                        }
                        return NonMixedSolutionSolution{0};
                }
        private:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer> const* AG_;
        };

        struct MinMixedSolution{
                size_t metric;
        };
        struct MinMixedSolutionCondition : AnyObserver{
                enum{ Debug = false };
                MinMixedSolutionCondition(std::shared_ptr<GameTree> gt,
                               GraphColouring<AggregateComputer> const& AG)
                        : gt_{gt}, AG_{&AG}
                {}
                boost::any Condition(StateType const& S)override{
                        auto gamma_vec = GammaVector(gt_, *AG_, S);
                        if( Debug ){
                                std::cout << "detail::to_string(gamma_vec) => " << detail::to_string(gamma_vec) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(gamma_vec))
                        }
                        if( ! IsMinMixedSolution(gamma_vec) )
                                return {};
                        size_t metric = 0;
                        for(auto _ : gamma_vec ){
                                metric += _ * _;
                        }
                        return MinMixedSolution{metric};
                }
        private:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer> const* AG_;
        };

        struct SolutionPrinter : AnyObserver{
                virtual boost::any Condition(StateType const& S)override{
                        for(size_t idx=0;idx!=S.size();++idx){
                                pretty_print_strat(S[idx][0], 4);
                        }
                        return {};
                }
        };


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
                obs.push_back(std::make_shared<MinMixedSolutionCondition>(gt, AG));
                obs.push_back(std::make_shared<NonMixedSolutionSolutionCondition>(gt, AG));

                std::vector<std::tuple<StateType, size_t, size_t> > solution_set;

                size_t k=0;
                for(;k!=9;++k){
                        GeometricLoopOptions opts;
                        opts.Delta = delta;
                        auto solution = GeometricLoopWithClamp(opts,
                                                               gt,
                                                               AG,
                                                               S,
                                                               obs);
                        if( auto ptr = boost::any_cast<NonMixedSolutionSolution>(&solution.Category)){
                                // we have a converged solution, but it's not minimal mixed
                                delta /= 2.0;
                                S = solution.S;
                                solution_set.emplace_back(S, 0, 100.0 - delta );
                        }
                        else if( auto ptr = boost::any_cast<MinMixedSolution>(&solution.Category)){
                                // we have found it
                                delta /= 2.0;
                                S = solution.S;
                                solution_set.emplace_back(solution.S, 1, ptr->metric );
                        } else{
                                // must not convert
                                break;
                        }

                }

                if( solution_set.empty()){
                        return boost::none;
                }

                std::cout << "solution_set.size() => " << solution_set.size() << "\n"; // __CandyPrint__(cxx-print-scalar,solution_set.size())
                using std::get;
                for(auto const& _ : solution_set ){
                        std::cout << "std::get<1>(_) => " << std::get<1>(_) << ", "; // __CandyPrint__(cxx-print-scalar,std::get<1>(_))
                        std::cout << "std::get<2>(_) => " << std::get<2>(_) << "\n"; // __CandyPrint__(cxx-print-scalar,std::get<2>(_))
                }
                std::sort( solution_set.begin(), solution_set.end(), [](auto&& l, auto&& r){
                        if( get<1>(l) != get<1>(r)){
                                return ( get<1>(l) < get<1>(r) );
                        }
                        return ( get<2>(l) > get<2>(r) );
                });
                return std::get<0>(solution_set.back());
        }
        boost::optional<StateType> AlgebraicSolver(holdem_binary_strategy_ledger_s& ledger, std::shared_ptr<GameTree> gt,
                                                   GraphColouring<AggregateComputer>& AG)
        {
                auto numerical_solution = NumericalSolver(ledger, gt, AG);


                return numerical_solution;

        }

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
                                        return iter->second.to_eigen_vv();
                                }
                        }

                        auto ledger_key = ".ledger/" + key;
                        holdem_binary_strategy_ledger_s ledger;
                        if( ! NoPersistent ){
                                ledger.try_load_or_default(ledger_key);
                        }


                        GraphColouring<AggregateComputer> AG = MakeComputer(gt);
                        auto sol = AlgebraicSolver(ledger, gt, AG);

                        if( sol ){
                                ss_.add_solution(key, *sol);
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

                        for(double eff = 2.0;eff - 1e-4 < 20.0; eff += 0.1 ){
                        //for(double eff = 11.0;eff - 1e-4 < 11.0; eff += 1.0 ){
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
