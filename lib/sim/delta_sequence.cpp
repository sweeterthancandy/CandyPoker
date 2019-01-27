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
                DeltaSequence(double factor_, double epsilon_, size_t stride_)
                        :factor(factor_),
                        epsilon(epsilon_),
                        stride(stride_)
                {}
                virtual void Execute(SolverContext& ctx)override
                {
                        auto gt = ctx.ArgGameTree();
                        auto root   = gt->Root();
                        
                        std::string uniq_key = gt->StringDescription() + "::DeltaSequence";

                        ctx.DeclUniqeKey(uniq_key);

                        StateType state0;
                        if(boost::optional<StateType> opt_state = ctx.RetreiveCandidateSolution()){
                                state0 = opt_state.get();
                        } else {
                                state0 = gt->MakeDefaultState();
                        }


                        auto AG   = ctx.ArgComputer();
                        
                        double delta = 0.01;
                        
                        auto S = state0;

                        std::vector<StateType> ledger;
                        for(size_t fails=0;fails!=3;){
                                PS_LOG(trace) << "delta => " << delta;
                                size_t counter = 0;
                                size_t max_counter = 1000;
                                for(;counter<max_counter;++counter){
                                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, delta);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - factor );
                                        if( S_counter == S )
                                                break;
                                        if( counter % stride == 0 ){
                                                computation_kernel::InplaceClamp(S, ClampEpsilon);
                                        }
                                }
                                if( counter != max_counter ){
                                        ledger.push_back(S);
                                        delta /= 2.0;
                                        continue;
                                }
                                PS_LOG(trace) << "delta=" << delta << " failed";
                                ++fails;
                                delta /= 2.0;

                        }
                        if( ledger.size() ){
                                ctx.EmitSolution(ledger.back());
                                return;
                        } else {
                                ctx.Message("Failed to converge :(889yh");
                                return;
                        }
                }
        private:
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
                virtual std::shared_ptr<Solver> Make(bpt::ptree const& params)const override{
                        double factor  = params.get<double>("factor");
                        double epsilon = params.get<double>("epsilon");
                        size_t stride  = params.get<size_t>("stride");

                        return std::make_shared<DeltaSequence>(factor, epsilon, stride);
                }
        };

        static SolverRegister<DeltaSequenceDecl> DeltaSequenceReg("delta-seq");

} // end namespace sim
} // end namespace ps
