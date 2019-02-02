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
                SimpleNumeric(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG, StateType state0,
                              double factor, size_t stride, double clamp_epsilon)
                        :gt_(gt),
                        AG_(AG),
                        state0_(state0),
                        factor_(factor),
                        stride_(stride),
                        clamp_epsilon_(clamp_epsilon)
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx)override
                {
                        auto root   = gt_->Root();
                        

                        SequenceConsumer sc;

                        auto S = state0_;

                        double delta = 0.00001;

                        for(;;){
                                PS_LOG(trace) << "delta => " << delta;
                                for(size_t counter=0;counter<1000;){
                                        for(size_t inner=0;inner!=stride_;++inner, ++counter){
                                                auto S_counter = computation_kernel::CounterStrategy(gt_, AG_, S, delta);
                                                computation_kernel::InplaceLinearCombination(S, S_counter, 1 - factor_ );
                                        }
                                        computation_kernel::InplaceClamp(S, clamp_epsilon_);

                                        DisplayStrategy(S,6);

                                        auto Sol = Solution::MakeWithDeps(gt_, AG_, S);

                                        switch( sc.Consume(std::move(Sol)) ){
                                        case SequenceConsumer::Ctrl_Rejected:
                                                break;
                                        case SequenceConsumer::Ctrl_Accepted:
                                                counter = 0;
                                                break;
                                        case SequenceConsumer::Ctrl_Perfect:
                                                return sc.AsState();
                                        }
                                }
                                if( sc && sc.AsSolution()->Level == 1 ){
                                        return sc.AsState();
                                }
                                delta *= 2;
                                if( delta > 0.05 ){
                                        return sc.AsState();
                                }
                        }
                }
        private:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer> AG_;
                StateType state0_;
                double factor_;
                size_t stride_;
                double clamp_epsilon_;
        };

        struct SimpleNumericDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                        using namespace std::string_literals;

                        V.DeclArgument("factor" , 0.05,
                                       "used for taking linear product, large faster, too large unstabe");
                        V.DeclArgument("stride" , static_cast<size_t>(20),
                                       "used for how many iterations before checking stoppage condition, "
                                       "larger faster, too large too slow");
                        V.DeclArgument("clamp-epsilon" , 1e-6,
                                       "used for clamping close to mixed strategies to non-mixed, "
                                       "too small slower convergence");
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {
                        auto factor        = args.get<double>("factor");
                        auto stride        = args.get<size_t>("stride");
                        auto clamp_epsilon = args.get<double>("clamp-epsilon");

                        return std::make_shared<SimpleNumeric>(gt, AG, inital_state, factor, stride, clamp_epsilon);
                }
        };

        static SolverRegister<SimpleNumericDecl> SimpleNumericReg("simple-numeric");

} // end namespace sim
} // end namespace ps
