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
                size_t stride{20};
                double clamp_epsilon{1e-6};
                double delta{0.0};

                void EmitDescriptions(SolverDecl::ArgumentVisitor& V)const{
                        V.DeclArgument("factor" , factor,
                                       "used for taking linear product, large faster, too large unstabe");
                        V.DeclArgument("stride" , stride,
                                       "used for how many iterations before checking stoppage condition, "
                                       "larger faster, too large too slow");
                        V.DeclArgument("clamp-epsilon" , clamp_epsilon,
                                       "used for clamping close to mixed strategies to non-mixed, "
                                       "too small slower convergence");
                        V.DeclArgument("delta" , delta, "forcing parameter");
                }
                void Read(bpt::ptree const& args){
                        factor        = args.get<double>("factor");
                        stride        = args.get<size_t>("stride");
                        clamp_epsilon = args.get<double>("clamp-epsilon");
                        delta         = args.get<double>("delta");
                }
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

        struct NumericSeqDecl : SolverDecl{

                struct NumericSeqArguments : SimpleNumericArguments{
                        using ImplType = SimpleNumericArguments;

                        size_t      ttl{10};
                        std::string sequence_type{"special"};

                        void EmitDescriptions(SolverDecl::ArgumentVisitor& V)const{
                                ImplType::EmitDescriptions(V);
                                V.DeclArgument("sequence-type", sequence_type, "how a sequence of solutions is choosen");
                                V.DeclArgument("ttl"          , ttl          , "time to live of sequence");
                        }
                        void Read(bpt::ptree const& args){
                                ImplType::Read(args);
                                ttl           = args.get<size_t>("ttl");
                                sequence_type = args.get<std::string>("sequence-type");
                        }
                };
                struct ConstantSequenceController : SimpleNumeric::Controller{

                        ConstantSequenceController(SequenceConsumer const& seq, size_t ttl)
                                : seq_{seq}
                                , ttl_{ttl}
                        {}

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
                        size_t ttl_;
                        size_t count_{0};
                };

                virtual void Accept(ArgumentVisitor& V)const override{
                        using namespace std::string_literals;

                        NumericSeqArguments proto;
                        proto.EmitDescriptions(V);


                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {

                        NumericSeqArguments sargs;
                        sargs.Read(args);

                        static std::unordered_map<
                                std::string, 
                                std::function<bool(Solution const&, Solution const&)>
                        > comps = {
                                { "level-sequence", [](auto const& head, auto const& candidate){ return head.Level < candidate.Level; } },
                                { "norm-sequence" , [](auto const& head, auto const& candidate){ return head.Norm < candidate.Norm; } } ,
                                { "special"       , [](auto const& head, auto const& candidate){ return head < candidate; } } 
                        };


                        auto comp = comps.find(sargs.sequence_type);
                        if( comp == comps.end() ){
                                BOOST_THROW_EXCEPTION(std::domain_error("no such sequence type of " + sargs.sequence_type ));
                        }

                        auto ctrl = std::make_shared<ConstantSequenceController>( SequenceConsumer( comp->second ), sargs.ttl );

                        auto solver = std::make_shared<SimpleNumeric>(gt, AG, inital_state, sargs);
                        solver->AddController(ctrl);

                        return solver;
                }
        };

        static SolverRegister<NumericSeqDecl> NumericSeqRec("numeric-sequence");
        
        
        
        struct TrailSolutionDecl : SolverDecl{
                struct TrailSolutionArguments : SimpleNumericArguments{
                        using ImplType = SimpleNumericArguments;

                        size_t level{10};

                        TrailSolutionArguments(){
                                factor        = 0.2;
                                clamp_epsilon = 1e-4;
                        }

                        void EmitDescriptions(SolverDecl::ArgumentVisitor& V)const{
                                ImplType::EmitDescriptions(V);
                                V.DeclArgument("level", level, "level to take first soltuin of");
                        }
                        void Read(bpt::ptree const& args){
                                ImplType::Read(args);
                                level = args.get<size_t>("level");
                        }
                };
                // this just returns the first solution which satisified cond(.)
                //
                // The idea is that we don't worry about non-convergence here
                struct TakeFirstController : SimpleNumeric::Controller{
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
                struct SequencePrinterController : SimpleNumeric::Controller{
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
                virtual void Accept(ArgumentVisitor& V)const override{
                        
                        TrailSolutionArguments proto;
                        proto.EmitDescriptions(V);
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {

                        TrailSolutionArguments sargs;
                        sargs.Read(args);

                        auto solver = std::make_shared<SimpleNumeric>(gt, AG, inital_state, sargs);
                        solver->AddController(std::make_shared<SequencePrinterController>());
                        solver->AddController(std::make_shared<SimpleNumeric::ProfileController>());
                        solver->AddController(std::make_shared<TakeFirstController>([lvl=sargs.level](auto const& sol){ return sol.Level <= lvl; }));

                        return solver;
                }
        };

        static SolverRegister<TrailSolutionDecl> TrailSolutionReg("trail-solution");

} // end namespace sim
} // end namespace ps
