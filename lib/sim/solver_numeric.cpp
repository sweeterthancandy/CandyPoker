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
                        virtual ApplyReturnType Apply(
                                size_t loop_count,
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)=0;
                };

                
                
                
                struct ProfileController : Controller{
                        virtual ApplyReturnType Apply(
                                size_t loop_count,
                                std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> AG,
                                SimpleNumericArguments& args, Solution const& solution)override
                        {
                                if( loop_count != 0 ){
                                        PS_LOG(trace) << "Loop took " << timer_.format();
                                }
                                timer_.start();
                                return {};
                        }
                private:
                        boost::timer::cpu_timer timer_;
                };
                
                


                SimpleNumeric( SimpleNumericArguments const& args)
                        : args_{args}
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx,
                                                           std::shared_ptr<GameTree> const& gt,
                                                           GraphColouring<AggregateComputer> const& AG,
                                                           StateType const& S0)override
                {
                        auto S = S0;
                                
                        auto solution0 = Solution::MakeWithDeps(gt, AG, S0);

                        for(auto& ctrl : controllers_ ){
                                auto opt_opt = ctrl->Apply(0, gt, AG, args_, solution0);
                                if( opt_opt ){
                                        PS_LOG(trace) << "skipping solver";
                                        return *opt_opt;
                                }
                        }

                        for(size_t loop_count{1};;++loop_count){
                                for(size_t inner=0;inner!=args_.stride;++inner){
                                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, args_.delta);
                                        computation_kernel::InplaceLinearCombination(S, S_counter, 1 - args_.factor );
                                }
                                computation_kernel::InplaceClamp(S, args_.clamp_epsilon);
                                        
                                auto solution = Solution::MakeWithDeps(gt, AG, S);
                                for(auto& ctrl : controllers_ ){
                                        auto opt_opt = ctrl->Apply(loop_count, gt, AG, args_, solution);
                                        if( opt_opt ){
                                                return *opt_opt;
                                        }
                                }

                        }
                }
                void AddController(std::shared_ptr<Controller> ctrl){
                        controllers_.push_back(ctrl);
                }
                virtual std::string StringDescription()const override{
                        std::stringstream sstr;
                        sstr << "SimpleNumeric{" << args_ << "}";
                        return sstr.str();
                }
        private:
                SimpleNumericArguments args_;
                std::vector<std::shared_ptr<Controller> > controllers_;
        };

        struct NumericSeqDecl : SolverDecl{

                struct NumericSeqArguments : SimpleNumericArguments{
                        using ImplType = SimpleNumericArguments;

                        size_t      ttl{10};
                        std::string sequence_type{"special"};
                        size_t      min_level{0};

                        void EmitDescriptions(SolverDecl::ArgumentVisitor& V)const{
                                ImplType::EmitDescriptions(V);
                                V.DeclArgument("sequence-type", sequence_type, "how a sequence of solutions is choosen");
                                V.DeclArgument("ttl"          , ttl          , "time to live of sequence");
                                V.DeclArgument("min-level" , min_level, "minoimum level, this can be used to have a large factor");
                        }
                        void Read(bpt::ptree const& args){
                                ImplType::Read(args);
                                ttl           = args.get<size_t>("ttl");
                                sequence_type = args.get<std::string>("sequence-type");
                                min_level     = args.get<size_t>("min-level");
                        }
                };
                struct ConstantSequenceController : SimpleNumeric::Controller{

                        ConstantSequenceController(SequenceConsumer const& seq, size_t ttl, size_t min_level)
                                : seq_{seq}
                                , ttl_{ttl}
                                , min_level_{min_level}
                        {}

                        virtual ApplyReturnType Apply(
                                size_t loop_count,
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

                                if( count_ >= ttl_){
                                        return seq_.AsOptState();
                                }

                                if( auto sol = seq_.AsOptSolution() ){
                                        if(sol->Level < min_level_ || sol->Level == 0){
                                                return boost::optional<StateType>{sol->S};
                                        }
                                }

                                return {};
                        }
                private:
                        SequenceConsumer seq_;
                        size_t ttl_;
                        size_t min_level_;
                        size_t count_{0};
                };

                virtual void Accept(ArgumentVisitor& V)const override{
                        using namespace std::string_literals;

                        NumericSeqArguments proto;
                        proto.EmitDescriptions(V);


                }
                virtual std::shared_ptr<Solver> Make( bpt::ptree const& args)const override
                {

                        NumericSeqArguments sargs;
                        sargs.Read(args);

                        static std::unordered_map<
                                std::string, 
                                std::function<bool(Solution const&, Solution const&)>
                        > comps = {
                                { "level-sequence", [](auto const& head, auto const& candidate){ return head.Level < candidate.Level; } },
                                { "total-sequence", [](auto const& head, auto const& candidate){ return head.Total < candidate.Total; } },
                                { "norm-sequence" , [](auto const& head, auto const& candidate){ return head.Norm < candidate.Norm; } } ,
                                { "special"       , [](auto const& head, auto const& candidate){ return head < candidate; } } 
                        };


                        auto comp = comps.find(sargs.sequence_type);
                        if( comp == comps.end() ){
                                BOOST_THROW_EXCEPTION(std::domain_error("no such sequence type of " + sargs.sequence_type ));
                        }

                        auto ctrl = std::make_shared<ConstantSequenceController>( SequenceConsumer( comp->second ), sargs.ttl, sargs.min_level );

                        auto solver = std::make_shared<SimpleNumeric>(sargs);
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
                                factor        = 0.1;
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
                                size_t loop_count,
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
                                size_t loop_count,
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
                virtual std::shared_ptr<Solver> Make(bpt::ptree const& args)const override
                {

                        TrailSolutionArguments sargs;
                        sargs.Read(args);

                        auto solver = std::make_shared<SimpleNumeric>(sargs);
                        solver->AddController(std::make_shared<SequencePrinterController>());
                        solver->AddController(std::make_shared<SimpleNumeric::ProfileController>());
                        solver->AddController(std::make_shared<TakeFirstController>([lvl=sargs.level](auto const& sol){ return sol.Level <= lvl; }));

                        return solver;
                }
        };

        static SolverRegister<TrailSolutionDecl> TrailSolutionReg("trail-solution");

} // end namespace sim
} // end namespace ps
