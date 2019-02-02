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
                                SimpleNumericArguments& args, Solution const& solution)=0;
                };

                struct ConstantSequenceController : Controller{
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {

                                switch( seq_.Consume(solution) ){
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

                                return {};
                        }
                private:
                        SequenceConsumer seq_;
                        size_t ttl_{10};
                        size_t count_{0};
                };
                
                // this just returns the first solution which satisified cond(.)
                //
                // The idea is that we don't worry about non-convergence here
                struct TakeFirstController : Controller{
                        using te_cond_type = std::function<bool(Solution const&)>;
                        explicit TakeFirstController(te_cond_type const& cond)
                                : cond_{cond}
                        {}
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {
                                if( cond_(solution) ){
                                        boost::optional<StateType> ret{solution.S};
                                        return ret;
                                }

                                return {};
                        }
                private:
                        te_cond_type cond_;
                };
                
                // just for debugging, print the sequence, but is not used.
                // The idea is to use this with TakeFirstController
                struct SequencePrinterController : Controller{
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {
                                seq_.Consume(solution);
                                return {};
                        }
                private:
                        SequenceConsumer seq_;
                };
                
                struct ProfileController : Controller{
                        virtual void Init(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args)override{
                                timer_.start();
                        }
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {
                                PS_LOG(trace) << "Loop took " << timer_.format();
                                timer_.start();
                                return {};
                        }
                private:
                        boost::timer::cpu_timer timer_;
                };
                
                
                /*
                        This is important. When we are running the numerical algorith,
                        we can't have the factor too large otherwise we potentially 
                        have an unstable solution, however at the start we can have a
                        very large factor. Of course things like this complicate the
                        algorithm so best have static conditions
                 */
                struct FactorTweakController : Controller{
                        virtual void Init(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args)override
                        {
                                saved_factor_ = args.factor;
                                args.factor = 0.2;
                                done_ = false;
                        }
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {
                                if( ! done_ ) {
                                        if( solution.Level < 10 ){
                                                args.factor = saved_factor_;
                                                done_ = true;
                                        }
                                }
                                return {};
                        }
                private:
                        double saved_factor_;
                        bool done_{false};
                };

                
                #if 0
                struct DeltaController : Controller{
                        virtual void Init(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args)override
                        {
                                args.delta = 0.00001;
                        }
                        virtual ApplyReturnType Apply(
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {
                                PS_LOG(trace) << "args = " << args;

                                switch( seq_.Consume(solution) ){
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
                #endif

                SimpleNumeric(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG, StateType state0,
                              SimpleNumericArguments const& args)
                        :gt_(gt),
                        AG_(AG),
                        state0_(state0),
                        args_{args}
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx)override
                {
                        auto S = state0_;

                        for(auto& ctrl : controllers_ ){
                                ctrl->Init(gt_, AG_, args_);
                        }
                        for(;;){
                                for(size_t inner=0;inner!=args_.stride;++inner){
                                        auto S_counter = computation_kernel::CounterStrategy(gt_, AG_, S, args_.delta);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - args_.factor );
                                }
                                computation_kernel::InplaceClamp(S, args_.clamp_epsilon);
                                        
                                DisplayStrategy(S,6);
                                
                                auto solution = Solution::MakeWithDeps(gt_, AG_, S);

                                for(auto& ctrl : controllers_ ){
                                        auto opt_opt = ctrl->Apply(gt_, AG_, args_, solution);
                                        if( opt_opt ){
                                                return *opt_opt;
                                        }
                                }

                        }
                }
                void AddController(std::shared_ptr<Controller> ctrl){
                        controllers_.push_back(ctrl);
                }
        private:
                std::shared_ptr<GameTree> gt_;
                GraphColouring<AggregateComputer> AG_;
                StateType state0_;
                SimpleNumericArguments args_;
                std::vector<std::shared_ptr<Controller> > controllers_;
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

                        auto solver = std::make_shared<SimpleNumeric>(gt, AG, inital_state, sargs);
                        solver->AddController(std::make_shared<SimpleNumeric::FactorTweakController>());
                        solver->AddController(std::make_shared<SimpleNumeric::ConstantSequenceController>());

                        return solver;
                }
        };

        static SolverRegister<SimpleNumericDecl> SimpleNumericReg("simple-numeric");
        
        
        
        struct TrailSolutionDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                        using namespace std::string_literals;
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {

                        SimpleNumericArguments sargs;
                        sargs.factor        = 0.2;
                        sargs.stride        = 30;
                        sargs.clamp_epsilon = 1e-5;
                        sargs.delta         = 0.0;


                        auto solver = std::make_shared<SimpleNumeric>(gt, AG, inital_state, sargs);
                        solver->AddController(std::make_shared<SimpleNumeric::SequencePrinterController>());
                        solver->AddController(std::make_shared<SimpleNumeric::ProfileController>());
                        solver->AddController(std::make_shared<SimpleNumeric::TakeFirstController>([](auto const& sol){ return sol.Level < 10; }));

                        return solver;
                }
        };

        static SolverRegister<TrailSolutionDecl> TrailSolutionReg("trail-solution");

} // end namespace sim
} // end namespace ps
