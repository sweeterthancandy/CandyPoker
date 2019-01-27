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

namespace ps{
namespace sim{
        struct Pipeline : Solver{
                Pipeline(std::shared_ptr<GameTree> gt_, GraphColouring<AggregateComputer> AG_, StateType state0_)
                        :gt(gt_),
                        AG(AG_),
                        state0(state0_)
                {}
                virtual boost::optional<StateType> Execute(SolverContext& ctx)override
                {
                        StateType S = state0;
                        for(auto const& p : vec_){
                                auto solver = SolverDecl::MakeSolver(p.first, gt, AG, S, p.second);
                                auto result = solver->Execute(ctx);
                                if( ! result )
                                        return {};
                                S = result.get();
                                DisplayStrategy(S,6);
                        }
                        return S;
                }
                void Next(std::string const& solver, std::string const& extra = std::string{}){
                        vec_.emplace_back(solver, extra);
                }
        private:
                std::shared_ptr<GameTree> gt;
                GraphColouring<AggregateComputer> AG;
                StateType state0;
                std::vector<std::pair<std::string, std::string> > vec_;
        };

        struct PipelineDecl : SolverDecl{
                virtual void Accept(ArgumentVisitor& V)const override{
                }
                virtual std::shared_ptr<Solver> Make( std::shared_ptr<GameTree> gt,
                                                      GraphColouring<AggregateComputer> AG,
                                                      StateType const& inital_state,
                                                      bpt::ptree const& args)const override
                {
                        auto ptr = std::make_shared<Pipeline>(gt, AG, inital_state);
                        ptr->Next("simple-numeric");
                        //ptr->Next("delta-seq");
                        ptr->Next("alpha");
                        return ptr;
                }
        };

        static SolverRegister<PipelineDecl> PipelineReg("pipeline");

} // end namespace sim
} // end namespace ps
