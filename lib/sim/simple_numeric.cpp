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
        struct SimpleNumeric : Solver{
                SimpleNumeric(std::shared_ptr<GameTree> gt_, GraphColouring<AggregateComputer> AG_, StateType state0_, double factor_, double epsilon_, size_t stride_)
                        :gt(gt_),
                        AG(AG_),
                        state0(state0_),
                        factor(factor_),
                        epsilon(epsilon_),
                        stride(stride_)
                {}
                virtual void Execute(SolverContext& ctx)override
                {
                        auto root   = gt->Root();
                        
                        std::string uniq_key = gt->StringDescription() + "::SimpleNumeric";
                        ctx.DeclUniqeKey(uniq_key);

                        auto S = state0;
                        for(size_t counter=0;counter!=400;){
                                for(size_t inner=0;inner!=stride;++inner, ++counter){
                                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, 0.0);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - factor );
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
        private:
                std::shared_ptr<GameTree> gt;
                GraphColouring<AggregateComputer> AG;
                StateType state0;
                double factor;
                double epsilon;
                size_t stride;
        };

        struct SimpleNumericDecl : SolverDecl{
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

                        return std::make_shared<SimpleNumeric>(gt, AG, inital_state, factor, epsilon, stride);
                }
        };

        static SolverRegister<SimpleNumericDecl> SimpleNumericReg("simple-numeric");

} // end namespace sim
} // end namespace ps
