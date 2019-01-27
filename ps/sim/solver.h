#ifndef PS_SIM_SOLVER_H
#define PS_SIM_SOLVER_H


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace bpt = boost::property_tree;

namespace ps{
namespace sim{

        struct SolverContext{
                virtual ~SolverContext()=default;

                // ultimatley the input
                virtual std::shared_ptr<GameTree>         ArgGameTree()=0;
                virtual GraphColouring<AggregateComputer>& ArgComputer()=0;
                /*
                        Along with the problem, its also expect to 
                        pass in extra string argument, most probaly
                        just a single JSON is appropirate.
                 */
                virtual std::vector<std::string>           ArgExtra()=0;

                // A large part of the solution is being able to run it for 6 hours,
                // then turn off the computer, and come back to it at a later date
                virtual void Message(std::string const& msg)=0;
                virtual void UpdateCandidateSolution(StateType const& S)=0;
                virtual boost::optional<StateType> RetreiveCandidateSolution()=0;

                virtual void EmitSolution(StateType const& S)=0;

                std::string const& UniqeKey()const{
                        BOOST_ASSERT( uniqe_key_.size() );
                        return uniqe_key_;
                }
                void DeclUniqeKey(std::string const& uniqe_key){
                        uniqe_key_ = uniqe_key;
                }
        private:
                std::string uniqe_key_;
        };


        struct Solver{
                virtual ~Solver()=default;
                virtual void Execute(SolverContext& ctx)=0;
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

                virtual std::shared_ptr<Solver> Make(bpt::ptree const& args)const=0;
                static SolverDecl* Get(std::string const& name){
                        auto ptr = Memory().find(name);
                        if( ptr == Memory().end() )
                                return nullptr;
                        return ptr->second.get();
                }


                static std::shared_ptr<Solver> MakeSolver(std::string const& name, std::string const& extra){
                        auto decl = Get(name);
                        if( ! decl )
                                BOOST_THROW_EXCEPTION(std::domain_error("solver doesn't exist " + name));
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
                                        if( _.second.data().empty() )
                                                continue;
                                        std::string k = _.first.data();
                                        std::string v = _.second.data();
                                        impl.args.put( k, v);
                                }
                        }

                        std::stringstream sstr;
                        bpt::write_json(sstr, impl.args, false);

                        PS_LOG(trace) << "Creating " << name << " with " << sstr.str();

                        return decl->Make(impl.args);
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
