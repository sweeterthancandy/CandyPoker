#ifndef PS_SIM_SOLVER_H
#define PS_SIM_SOLVER_H


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace bpt = boost::property_tree;

namespace ps{
namespace sim{



        struct SolverContext{
                virtual ~SolverContext()=default;

                // A large part of the solution is being able to run it for 6 hours,
                // then turn off the computer, and come back to it at a later date
                virtual void Message(std::string const& msg)=0;
        };


        struct Solver{
                virtual ~Solver()=default;
                virtual boost::optional<StateType> Execute(SolverContext& ctx,
                                                           std::shared_ptr<GameTree> const& gt,
                                                           GraphColouring<AggregateComputer> const& AG,
                                                           StateType const& S0)=0;
                // for debugging
                virtual std::string StringDescription()const=0;
        };

        struct AggregateSolver : Solver{
                virtual boost::optional<StateType> Execute(SolverContext& ctx,
                                                           std::shared_ptr<GameTree> const& gt,
                                                           GraphColouring<AggregateComputer> const& AG,
                                                           StateType const& S0)override
                {
                        StateType S = S0;
                        for(auto const& sub : subs_ ){
                                PS_LOG(trace) << "Executing sub solver " << sub->StringDescription();
                                auto ret = sub->Execute(ctx, gt, AG, S);
                                if( ! ret ){
                                        return {};
                                }
                                S = ret.get();
                        }
                        return S;
                }
                void Add(std::shared_ptr<Solver> const& ptr){
                        subs_.push_back(ptr);
                }
                virtual std::string StringDescription()const override{
                        std::stringstream sstr;
                        sstr << "AggregateSolver{";
                        for(size_t idx=0;idx!=subs_.size();++idx){
                                if( idx != 0 ) sstr << ", ";
                                sstr << subs_[idx]->StringDescription();
                        }
                        sstr << "}";
                        return sstr.str();
                }
        private:
                std::vector<std::shared_ptr<Solver> > subs_;
        };
        

        
        struct SolverDecl{
                struct ArgumentVisitor{
                        virtual ~ArgumentVisitor()=default;
                        using ArgumentType = boost::variant<size_t, double, std::string>;
                        virtual void DeclArgument(std::string const& name,
                                                  ArgumentType const& default_value,
                                                  std::string const& description)=0;
                };
                virtual void Accept(ArgumentVisitor&)const{}

                virtual std::shared_ptr<Solver> Make( bpt::ptree const& args)const=0;
                

                ////////////////////////// This is the static sutff //////////////////////////////////
                static std::shared_ptr<SolverDecl> Get(std::string const& name){

                        // we have two namespaces, the memory for the underlying solvers, and
                        // also a config file
                        auto ptr = Memory().find(name);
                        if( ptr != Memory().end() )
                                return ptr->second;


                        return {};
                }


                static std::shared_ptr<Solver> MakeSolver(std::string const& name, std::string const& extra){
                        auto decl = Get(name);
                        if( !! decl ){
                                struct Impl : ArgumentVisitor{
                                        virtual void DeclArgument(std::string const& name,
                                                                  ArgumentType const& default_value,
                                                                  std::string const& description)override
                                        {
                                                boost::apply_visitor([&](auto&& _){
                                                        args.put(name, _);
                                                }, default_value);
                                        }
                                        bpt::ptree args;
                                };
                                Impl impl;
                                decl->Accept(impl);

                                if( extra.size() ){
                                        bpt::ptree args;

                                        std::stringstream sstr;
                                        sstr << extra;
                                        bpt::read_json(sstr, args);

                                        for(auto const& _ : args){
                                                if( _.second.empty() ){
                                                        std::string k = _.first.data();
                                                        std::string v = _.second.data();
                                                        impl.args.put( k, v);
                                                } else {
                                                        impl.args.put_child(_.first.data(), _.second);
                                                }
                                        }
                                }

                                std::stringstream sstr;
                                bpt::write_json(sstr, impl.args, false);

                                PS_LOG(trace) << "Creating " << name << " with " << sstr.str();

                                return decl->Make(impl.args);
                        } else {
                                // we, perhaps this is a user defined solver
                                try{
                                        std::ifstream ifstr(".ps.solvers.json");
                                        if( ifstr.is_open()){
                                                bpt::ptree root;
                                                bpt::read_json(ifstr, root);

                                                if( auto opt = root.get_child_optional(name) ){
                                                        bpt::write_json(std::cout, opt.get());
                                                        auto agg = std::make_shared<AggregateSolver>();
                                                        for(auto const& sub_desc : opt.get() ){
                                                                bpt::ptree const& head = sub_desc.second;
                                                                std::string solver = head.get<std::string>("solver");
                                                                std::string args;
                                                                if( auto args_opt = head.get_child_optional("args") ){
                                                                        std::stringstream tmp;
                                                                        bpt::write_json(tmp, args_opt.get(), false);
                                                                        args = tmp.str();
                                                                }
                                                                PS_LOG(trace) << "solver => " << solver;
                                                                PS_LOG(trace) << "args => "  << args;
                                                                auto sub = SolverDecl::MakeSolver(solver, args);
                                                                if( ! sub ){
                                                                        BOOST_THROW_EXCEPTION(std::domain_error("bad solver " + solver + " with args " + args));
                                                                }
                                                                agg->Add(sub);
                                                        }
                                                        return agg;
                                                }
                                        }
                                } catch(std::exception const& e){
                                        PS_LOG(error) << "failed to read config " << e.what();
                                }
                        }
                        BOOST_THROW_EXCEPTION(std::domain_error("solver doesn't exist " + name));
                }
        private:
                template<class T>
                friend struct SolverRegister;
                static std::unordered_map<std::string, std::shared_ptr<SolverDecl> >& Memory(){
                        static std::unordered_map<std::string, std::shared_ptr<SolverDecl> > M;
                        return M;
                }
        };
        template<class T>
        struct SolverRegister{
                SolverRegister(std::string const& name){
                        SolverDecl::Memory()[name] = std::make_shared<T>();
                }
        };


        

} // end namespace sim
} // end namespace ps

#endif // PS_SIM_SOLVER_H
