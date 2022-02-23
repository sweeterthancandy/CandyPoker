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
#include "ps/detail/popcount.h"

#include "ps/sim/computer.h"
#include "ps/sim/game_tree.h"
#include "ps/sim/computer_factory.h"
#include "ps/sim/_extra.h"
#include "ps/sim/solver.h"

#include <boost/any.hpp>
#include <numeric>
#include <boost/timer/timer.hpp>

namespace ps{
namespace sim{
        struct PermutationSolverArguments{
                double clamp_epsilon{1e-6};
                size_t grid_size    {11};
                size_t max_popcount {1};
                size_t dbg_use_threads{true};
                // this is stop wasting CPU time
                size_t max_evaluations{1000};


                void EmitDescriptions(SolverDecl::ArgumentVisitor& V)const{
                        V.DeclArgument("clamp-epsilon" , clamp_epsilon,
                                       "used for clamping close to mixed strategies to non-mixed, "
                                       "too small slower convergence");
                        V.DeclArgument("grid-size" , grid_size, "the size of each grid");
                        V.DeclArgument("max-popcount" , max_popcount, "number of freedoms, O(n!)");
                        V.DeclArgument("dbg-use-threads" , dbg_use_threads, "development aid");
                        V.DeclArgument("max-evaluations", max_evaluations, "upper limit of where to fail");
                }
                void Read(bpt::ptree const& args){
                        grid_size       = args.get<double>("grid-size");
                        max_popcount    = args.get<size_t>("max-popcount");
                        clamp_epsilon   = args.get<double>("clamp-epsilon");
                        dbg_use_threads = args.get<size_t>("dbg-use-threads");
                        max_evaluations = args.get<size_t>("max-evaluations");
                }
        };
        /*
         * This is a wrapper around StateType. This solves the problem
         * of computing the counter strategy and EV in observer functions.
         */
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

        inline std::vector<MixedSolutionDescription> MakeMixedSolutionDescription(StateType const& S, StateType const& CS){
                /*
                        Here we want to construct a vector which indicate all the mixed strategis.
                        For example if every was push/fold for hero/villian excep A2o for Hero, and 
                        67o and 79o for villian, we would have
                                        ({A2o}, {67o, 79o}).
                 */
                std::vector<MixedSolutionDescription> mixed_info;
                for(size_t idx=0;idx!=S.size();++idx){
                        auto const& c = CS[idx][0];
                        auto const& t = S[idx][0];
                        std::vector<size_t> mixed;
                        for(size_t cid=0;cid!=169;++cid){
                                if( t[cid] == c[cid] )
                                        continue;
                                mixed.push_back(cid);
                        }
                        if( mixed.empty() )
                                continue;
                        mixed_info.emplace_back(idx, std::move(mixed));
                }
                return mixed_info;
        }

        struct PermutationSolver : Solver{


                PermutationSolver( PermutationSolverArguments const& args)
                        : args_{args}
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx,
                                                           std::shared_ptr<GameTree> const& gt,
                                                           GraphColouring<AggregateComputer> const& AG,
                                                           StateType const& S0)override
                {

                        PS_LOG(trace) << "----------------- PermutationSolver ---------------";

                        assert( args_.grid_size != 1 );
                        
                        Solution Sol = Solution::MakeWithDeps(gt, AG, S0);
                        auto const& S = Sol.S;

                        auto const& Counter = Sol.Counter;

                        
                        std::vector<MixedSolutionDescription> mixed_info = MakeMixedSolutionDescription(S, Counter);
                        #if 0
                        /*
                                Here we want to construct a vector which indicate all the mixed strategis.
                                For example if every was push/fold for hero/villian excep A2o for Hero, and 
                                67o and 79o for villian, we would have
                                                ({A2o}, {67o, 79o}).
                         */
                        for(size_t idx=0;idx!=S.size();++idx){
                                auto const& c = Counter[idx][0];
                                auto const& t = S[idx][0];
                                std::vector<size_t> mixed;
                                for(size_t cid=0;cid!=169;++cid){
                                        if( t[cid] == c[cid] )
                                                continue;
                                        mixed.push_back(cid);
                                }
                                if( mixed.empty() )
                                        continue;
                                mixed_info.emplace_back(idx, std::move(mixed));
                        }
                        
                        for(auto const& mi : mixed_info){
                                std::cout << mi << "\n";
                        }
                        #endif

                        using factor_vector_type = std::vector<size_t>;
                        using factor_set_type = std::vector<factor_vector_type>;
                        using factor_family = std::vector<factor_set_type>;

                        std::vector<factor_family> family_vec;

                        for(auto const& mi : mixed_info){
                                
                                factor_vector_type proto(mi.mixed.size(),1);

                                factor_family family;

                                family.push_back(factor_set_type{proto});

                                factor_set_type s;
                                for(size_t idx=0;idx!=mi.mixed.size();++idx){
                                        auto next = proto;
                                        next[idx] = args_.grid_size;
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

                        for(size_t level = 0; level <= args_.max_popcount; ++level ){
                                for(size_t mask = 0; mask != upper_bound; ++mask ){
                                        if( detail::popcount(mask) != level )
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
                        for(auto const& realization : boost::range::unique(boost::range::sort(realizations))){
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


                        std::vector<Solution> solution_candidates;

                        PS_LOG(trace) << "SV_family.size() => " << SV_family.size();

                        // do nothing rather than waste computation on something not tangible
                        if( SV_family.size() > args_.max_evaluations ){
                                PS_LOG(warning) << "Failing as we have " << SV_family.size() << " evaluations to perfom";
                                return {};
                        }

                        #if 0
                        for(size_t idx=0;idx!=SV_family.size();++idx){
                                std::cout << "idx => " << idx << " out of " << SV_family.size() << "\n"; // __CandyPrint__(cxx-print-scalar,idx)
                                auto Q = SV_family[idx];
                                computation_kernel::InplaceClamp(Q, args_.clamp_epsilon);
                                auto candidate = Solution::MakeWithDeps(gt, AG, Q);
                                solution_candidates.push_back(candidate);
                        }
                        #else
                        std::vector<std::function<Solution()> > tasks;
                        for(size_t idx=0;idx!=SV_family.size();++idx){
                                auto atom = [&,idx](){
                                        auto Q = SV_family[idx];
                                        computation_kernel::InplaceClamp(Q, args_.clamp_epsilon);
                                        auto candidate = Solution::MakeWithDeps(gt, AG, Q);
                                        return candidate;
                                };
                                tasks.push_back(atom);
                        }
                        if( args_.dbg_use_threads ){
                                std::vector<std::future<Solution> > futs;
                                for(auto& t : tasks ){
                                        //futs.push_back( std::async(std::launch::async, t) );
                                        futs.push_back( std::async(t) );
                                }
                                for(auto& f : futs){
                                        solution_candidates.push_back(f.get());
                                }
                        } else {
                                for(auto& t : tasks ){
                                        solution_candidates.push_back(t());
                                }
                        }
                        #endif

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

                        if( solution_candidates.empty() )
                               return {};
                        return solution_candidates.front().S;
                        #if 0
                        if( solution_candidates.size() && solution_candidates.front().Level <= 1 ){
                                auto const& best = solution_candidates.front();
                                std::cout << "detail::to_string(best.Gamma) => " << detail::to_string(best.Gamma) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(best.Gamma))
                                return best.S;
                        } else {
                                ctx.Message("no best :(");
                        }
                        return {};
                        #endif
                }
                virtual std::string StringDescription()const override{
                        std::stringstream sstr;
                        sstr << "PermutationSolver{}";
                        return sstr.str();
                }
        private:
                PermutationSolverArguments args_;
        };
        
        struct PermutationSolverDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                        PermutationSolverArguments proto;
                        proto.EmitDescriptions(V);
                }
                virtual std::shared_ptr<Solver> Make( bpt::ptree const& args)const override
                {
                        PermutationSolverArguments bargs;
                        bargs.Read(args);
                        return std::make_shared<PermutationSolver>(bargs);
                }
        };

        static SolverRegister<PermutationSolverDecl> PermutationSolverReg("permutation");
        
        
        
        struct SinglePermutationSolver : Solver{

                virtual boost::optional<StateType> Execute(SolverContext& ctx,
                                                           std::shared_ptr<GameTree> const& gt,
                                                           GraphColouring<AggregateComputer> const& AG,
                                                           StateType const& S0)override
                {

                        PS_LOG(trace) << "----------------- SinglePermutationSolver ---------------";

                        SequenceConsumer seq;
                        seq.Consume(Solution::MakeWithDeps(gt, AG, S0));
                        for(size_t loop_count=0;;++loop_count){

                                std::cout << "loop_count => " << loop_count << "\n"; // __CandyPrint__(cxx-print-scalar,loop_count)

                                auto opt_sol = seq.AsOptSolution();
                                BOOST_ASSERT( opt_sol );
                                Solution const& Sol = opt_sol.get();

                                auto const& S  = Sol.S;
                                auto const& CS = Sol.Counter;

                                std::vector<MixedSolutionDescription> mixed_info = MakeMixedSolutionDescription(S, CS);

                                std::vector<StateType> candidates;

                                for(auto const& mi : mixed_info ){
                                        for(auto const& cid : mi.mixed ){
                                                auto next = S;
                                                next[mi.player_index][0][cid] = 1.0;
                                                next[mi.player_index][1][cid] = 0.0;
                                                candidates.push_back(next);
                                                next[mi.player_index][0][cid] = 0.0;
                                                next[mi.player_index][1][cid] = 1.0;
                                                candidates.push_back(next);
                                        }
                                }
                                std::cout << "mixed_info.size() => " << mixed_info.size() << "\n"; // __CandyPrint__(cxx-print-scalar,mixed_info.size())
                                std::vector<std::function<Solution()> > tasks;
                                for(auto const& cand : candidates){
                                        auto atom = [&]()->Solution{
                                                auto solution = Solution::MakeWithDeps(gt, AG, cand);
                                                return solution;
                                        };
                                        tasks.emplace_back(atom);
                                }
                                std::cout << "tasks.size() => " << tasks.size() << "\n"; // __CandyPrint__(cxx-print-scalar,tasks.size())
                                std::vector<std::future<Solution> > futs;
                                for(auto& t : tasks ){
                                        futs.push_back( std::async(std::launch::async, t) );
                                }
                                std::vector<Solution> solution_candidates;
                                for(auto& f : futs){
                                        solution_candidates.push_back(f.get());
                                }
                                #if 0
                                std::mutex mtx;
                                auto child = [&]()mutable{
                                        for(;;){
                                                
                                                mtx.lock();
                                                if( tasks.empty() ){
                                                        mtx.unlock();
                                                        return;
                                                }
                                                auto fut = std::move(tasks.back());
                                                tasks.pop_back();
                                                mtx.unlock();
                                                fut();
                                        }
                                };
                                std::vector<std::thread> tg;
                                for(size_t idx=0;idx!=std::thread::hardware_concurrency();++idx){
                                        tg.emplace_back(child);
                                }
                                for(auto& _ : tg){
                                        _.join();
                                }
                                #endif
                                
                                bool do_break = true;
                                std::cout << "HEAD " << Sol.Total << "\n"; // __CandyPrint__(cxx-print-scalar,Sol.Total)
                                for(auto& _ : solution_candidates){
                                        switch(seq.Consume(_)){
                                        case SequenceConsumer::Ctrl_Rejected:
                                                break;
                                        case SequenceConsumer::Ctrl_Accepted:
                                                std::cout << "Accepted\n";
                                                do_break = false;
                                                break;
                                        case SequenceConsumer::Ctrl_Perfect:
                                                return seq.AsOptState();
                                        }
                                }

                                if( do_break ){
                                        break;
                                }
                        }
                        return seq.AsOptState();
                }
                virtual std::string StringDescription()const override{
                        std::stringstream sstr;
                        sstr << "SinglePermutationSolver{}";
                        return sstr.str();
                }
        };
        
        struct SinglePermutationSolverDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                }
                virtual std::shared_ptr<Solver> Make( bpt::ptree const& args)const override
                {
                        return std::make_shared<SinglePermutationSolver>();
                }
        };

        static SolverRegister<SinglePermutationSolverDecl> SinglePermutationSolverReg("single-permutation");

} // end namespace sim
} // end namespace ps
