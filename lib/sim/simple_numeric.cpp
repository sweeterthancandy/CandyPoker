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
        

        struct SimpleNumericArguments{
                friend std::ostream& operator<<(std::ostream& ostr, SimpleNumericArguments const& self){
                        ostr << "factor = " << self.factor;
                        ostr << ", stride = " << self.stride;
                        ostr << ", clamp_epsilon = " << self.clamp_epsilon;
                        ostr << ", delta = " << self.delta;
                        return ostr;
                }
                double factor{0.05};
                size_t stride{10};
                double clamp_epsilon{1e-6};
                double delta{0.0};
        };

        struct SimpleNumeric : Solver{
                        
                struct Controller{
                        virtual ~Controller(){}

                        using ApplyReturnType = boost::optional<
                                boost::optional<
                                        StateType
                                >
                        >;
                        virtual void Init(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args){}
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, StateType const& S)=0;
                };

                struct ConstantController : Controller{
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, StateType const& S)override
                        {
                                auto Sol = Solution::MakeWithDeps(gt, AG, S);

                                switch( seq_.Consume(std::move(Sol)) ){
                                case SequenceConsumer::Ctrl_Rejected:
                                        ++count_;
                                        break;
                                case SequenceConsumer::Ctrl_Accepted:
                                        count_ = 0;
                                        break;
                                case SequenceConsumer::Ctrl_Perfect:
                                        return seq_.AsOptState();
                                }

                                if( count_ >= ttl_ ){
                                        return seq_.AsOptState();
                                }
                                std::cout << "count_ => " << count_ << "\n"; // __CandyPrint__(cxx-print-scalar,count_)

                                return {};
                        }
                private:
                        SequenceConsumer seq_;
                        size_t ttl_{10};
                        size_t count_{0};
                };
                
                struct DeltaController : Controller{
                        virtual void Init(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args)override
                        {
                                args.delta = 0.00001;
                                saved_factor_ = args.factor;
                                args.factor = 0.2;
                        }
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, StateType const& S)override
                        {
                                PS_LOG(trace) << "args = " << args;
                                auto Sol = Solution::MakeWithDeps(gt, AG, S);
                                size_t lvl = Sol.Level;

                                switch( seq_.Consume(std::move(Sol)) ){
                                case SequenceConsumer::Ctrl_Rejected:
                                        ++count_;
                                        break;
                                case SequenceConsumer::Ctrl_Accepted:
                                        count_ = 0;

                                        if( lvl < 10 ){
                                                args.factor = saved_factor_;
                                        }
                                        break;
                                case SequenceConsumer::Ctrl_Perfect:
                                        return seq_.AsOptState();
                                }

                                if( count_ >= ttl_ ){
                                        count_ = 0;
                                        args.delta *= 2.0;
                                        if( args.delta > 0.05 ){
                                                return seq_.AsOptState();
                                        }
                                        PS_LOG(trace) << "delta increase to " << args.delta;
                                }

                                return {};
                        }
                private:
                        SequenceConsumer seq_;
                        size_t ttl_{10};
                        size_t count_{0};

                        double saved_factor_;
                };

                SimpleNumeric(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG, StateType state0,
                              SimpleNumericArguments const& args, std::shared_ptr<Controller> ctrl)
                        :gt_(gt),
                        AG_(AG),
                        state0_(state0),
                        args_{args},
                        ctrl_{ctrl}
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx)override
                {
                        auto S = state0_;

                        ctrl_->Init(gt_, AG_, args_);
                        for(;;){
                                for(size_t inner=0;inner!=args_.stride;++inner){
                                        auto S_counter = computation_kernel::CounterStrategy(gt_, AG_, S, args_.delta);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - args_.factor );
                                }
                                computation_kernel::InplaceClamp(S, args_.clamp_epsilon);
                                        
                                DisplayStrategy(S,6);

                                auto opt_opt = ctrl_->Apply(gt_, AG_, args_, S);
                                if( opt_opt )
                                        return *opt_opt;

                        }
                }
        private:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer> AG_;
                StateType state0_;
                SimpleNumericArguments args_;
                std::shared_ptr<Controller> ctrl_;
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
                        V.DeclArgument("delta" , 0.0, "delta");
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {

                        SimpleNumericArguments sargs;
                        sargs.factor        = args.get<double>("factor");
                        sargs.stride        = args.get<size_t>("stride");
                        sargs.clamp_epsilon = args.get<double>("clamp-epsilon");
                        sargs.delta         = args.get<size_t>("delta");

                        auto ctrl = std::make_shared<SimpleNumeric::DeltaController>();

                        auto solver = std::make_shared<SimpleNumeric>(gt, AG, inital_state, sargs, ctrl);

                        return solver;
                }
        };

        static SolverRegister<SimpleNumericDecl> SimpleNumericReg("simple-numeric");

} // end namespace sim
} // end namespace ps
