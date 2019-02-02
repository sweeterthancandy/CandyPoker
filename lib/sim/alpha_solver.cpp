
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

#include <numeric>


#include <boost/timer/timer.hpp>
namespace ps{
namespace sim{
        struct AlphaSolver : Solver{
                enum ResultType{
                        FoundGamma,
                        FoundPerfect,
                        LoopOverFlow,
                };
                AlphaSolver(std::shared_ptr<GameTree> gt_, GraphColouring<AggregateComputer> AG_, StateType state0_)
                        :gt(gt_),
                        AG(AG_),
                        state0(state0_)
                {}
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
                virtual boost::optional<StateType> Execute(SolverContext& ctx)override{



                        double ClampEpsilon{1e-6};


                        // find best one from the ledger
                                        
                        ctx.Message("Doing brute force search...");
                        
                        Solution Sol = Solution::MakeWithDeps(gt, AG, state0);
                        auto const& S = Sol.S;

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

                        // do nothing rather than waste computation on something not tangible
                        if( SV_family.size() > 1000 )
                                return {};

                        boost::optional<Solution> best;

                        for(size_t idx=0;idx!=SV_family.size();++idx){
                                std::cout << "idx => " << idx << " out of " << SV_family.size() << "\n"; // __CandyPrint__(cxx-print-scalar,idx)
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
        private:
                std::shared_ptr<GameTree> gt;
                GraphColouring<AggregateComputer> AG;
                StateType state0;
        };
        
        struct AlphaSolverDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {
                        return std::make_shared<AlphaSolver>(gt, AG, inital_state);
                }
        };

        static SolverRegister<AlphaSolverDecl> AlphaSolverReg("alpha");

} // end namespace sim
} // end namespace ps
