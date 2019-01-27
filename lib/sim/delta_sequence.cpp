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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace bpt = boost::property_tree;

#include <numeric>


#include <boost/timer/timer.hpp>

namespace ps{
namespace sim{
        struct DeltaSequence : Solver{
                DeltaSequence(std::shared_ptr<GameTree> gt_, GraphColouring<AggregateComputer> AG_, StateType state0_, double factor_, double epsilon_, size_t stride_)
                        :gt(gt_),
                        AG(AG_),
                        state0(state0_),
                        factor(factor_),
                        epsilon(epsilon_),
                        stride(stride_)
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx)override
                {
                        auto root   = gt->Root();
                        
                        double delta = 0.1;
                        
                        auto S = state0;


                        SequenceConsumer sc;

                        std::vector<StateType> ledger;
                        enum{ MaxFails = 1 };
                        for(size_t fails=0;fails!=MaxFails;){
                                PS_LOG(trace) << "delta => " << delta;
                                size_t counter = 0;
                                size_t max_counter = 1000;
                                for(;counter<max_counter;++counter){
                                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, delta);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - factor );
                                        if( S_counter == S ){
                                                break;
                                        }
                                        if( counter % stride == 0 ){
                                                computation_kernel::InplaceClamp(S, ClampEpsilon);
                                        }
                                }
                                if( counter != max_counter ){
                                        if( ledger.size() > 4 ){
                                                if( ledger[ledger.size()-1] == S &&
                                                    ledger[ledger.size()-2] == S &&
                                                    ledger[ledger.size()-3] == S ){
                                                        return S;
                                                }
                                        }
                                        ledger.push_back(S);
                                        delta /= 2.0;
                                        continue;
                                }
                                PS_LOG(trace) << "delta=" << delta << " failed";
                                ++fails;
                                delta /= 2.0;

                        }
                        return S;
                        if( ledger.size() ){
                                return ledger.back();
                        }
                        ctx.Message("Failed to converge :(889yh");
                        return {};
                }
        private:
                std::shared_ptr<GameTree> gt;
                GraphColouring<AggregateComputer> AG;
                StateType state0;
                double factor;
                double epsilon;
                size_t stride;
                double ClampEpsilon{1e-6};
        };

        struct DeltaSequenceDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                        double const default_factor  = 0.05;
                        double const default_epsilon = 0.00001;
                        size_t const default_stride  = 10;

                        V.DeclArgument("factor" , default_factor, "used for taking linear product");
                        V.DeclArgument("epsilon", default_epsilon, "used for stoppage condition");
                        V.DeclArgument("stride" , default_stride,
                                       "used for how many iterations before checking stoppage condition");
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {
                        double factor  = args.get<double>("factor");
                        double epsilon = args.get<double>("epsilon");
                        size_t stride  = args.get<size_t>("stride");

                        return std::make_shared<DeltaSequence>(gt, AG, inital_state, factor, epsilon, stride);
                }
        };

        static SolverRegister<DeltaSequenceDecl> DeltaSequenceReg("delta-seq");

} // end namespace sim
} // end namespace ps
