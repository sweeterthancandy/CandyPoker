#ifndef PS_SIM_SOLVER_H
#define PS_SIM_SOLVER_H

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
                static Solver* Get(std::string const& name){
                        auto ptr = Memory().find(name);
                        if( ptr == Memory().end() )
                                return nullptr;
                        return ptr->second.get();
                }
        private:
                template<class T>
                friend struct SolverRegister;
                static std::unordered_map<std::string, std::shared_ptr<Solver> >& Memory(){
                        static std::unordered_map<std::string, std::shared_ptr<Solver> > M;
                        return M;
                }
        };
        template<class T>
        struct SolverRegister{
                SolverRegister(std::string const& name){
                        Solver::Memory()[name] = std::make_shared<T>();
                }
        };

} // end namespace sim
} // end namespace ps

#endif // PS_SIM_SOLVER_H
