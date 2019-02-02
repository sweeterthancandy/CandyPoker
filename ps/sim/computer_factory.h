#ifndef PS_SIM_COMPUTER_FACTORY_H
#define PS_SIM_COMPUTER_FACTORY_H

#include <boost/scope_exit.hpp>
#include <boost/timer/timer.hpp>


namespace ps{
namespace sim{

        using detail::GNode;
        using detail::GEdge;
        using detail::Graph;
        using detail::GraphColouring;

        
        /*
         * For solving poker push-fold, we need some representation of a numerical solution.
         * For push fold, I'm representing the hu solution as
         *                  {0,1} x {push,fold} x {0,1}^169,
         * corresponding to sb push/fold choice for 0, and bb call/fold decision given sb
         * push for 1. either [0][0][JJ] is the probailty that sb push's JJ, whilst
         * [1][1][QQ] is the probabily that bb calls with QQ.
         *    I capture that inside the the std::vector<Index>, which corresponds how to
         * assign a probability to a path. We want to evaluate the probability of 
         * event (terminal) happening for a certain deal, ei
         *          P(e=PP|cv={AKs,33}) = S[0][0][AKs]*S[1][0][33].
         *    Now we will always have
         *          P(cv={AKs,33}) = P(e=PP,cv={AKs,33}) + P(e=PF,cv={AKs,33}) + P(e=F,cv={AKs,33}) = 1,
         *    Ie that it forms a probability distribution.
         */
        struct IndexMakerConcept{
                virtual ~IndexMakerConcept()=default;
                virtual std::vector<Index> MakeIndex(std::vector<GEdge const*> const& path, holdem_class_vector const& cv)const=0;
        };

        struct MakerConcept{
                virtual ~MakerConcept()=default;
                virtual void Emit(IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const=0;
                virtual std::vector<std::string> to_string_line()const=0;

                static void Display( std::vector<std::shared_ptr<MakerConcept> > const& v){
                        std::vector<Pretty::LineItem> lines;
                        lines.push_back(std::vector<std::string>{"Path", "Impl", "Active", "Pot"});
                        lines.push_back(Pretty::LineBreak);
                        for(auto const& ptr : v){
                                lines.push_back( ptr->to_string_line());
                        }
                        Pretty::RenderTablePretty(std::cout, lines);
                }
        };
        inline std::string path_to_string(std::vector<GEdge const*> const& path){
                std::stringstream pathsstr;
                pathsstr << path.front()->From()->Name();
                for(auto p : path){
                        pathsstr << "," << p->To()->Name();
                }
                return pathsstr.str();
        }

        struct StaticEmit : MakerConcept{
                StaticEmit(std::shared_ptr<Computer> comp, std::vector<GEdge const*> path, std::vector<size_t> const& active, Eigen::VectorXd pot)
                        :comp_{comp},
                        path_(path),
                        active_(active),
                        pot_(pot)
                {}
                virtual void Emit(IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const override{
                        
                        auto index = im->MakeIndex(path_, cv); 
                        
                        Eigen::VectorXd v(pot_.size());
                        v.fill(0);
                        v -= pot_;

                        if( active_.size() == 1 ){
                                v[active_[0]] += pot_.sum();
                        } else {
                                holdem_class_vector auxcv;
                                for(auto idx : active_ ){
                                        auxcv.push_back(cv[idx]);
                                }
                                auto ev = cache->LookupVector(auxcv);
                                Eigen::VectorXd evaux(cv.size());
                                evaux.fill(0);
                                size_t ev_idx = 0;
                                for(auto idx : active_ ){
                                        evaux[idx] = ev[ev_idx];
                                        ++ev_idx;
                                }
                                evaux *= pot_.sum();

                                v += evaux;
                        }

                        comp_->Emplace(cv, p, index, v);
                }
                virtual std::vector<std::string> to_string_line()const override{
                        std::vector<std::string> line;
                        line.push_back(path_to_string(path_));
                        if( active_.size() == 1 ){
                                line.push_back("Static");
                        } else {
                                line.push_back("Eval");
                        }
                        line.push_back(detail::to_string(active_));
                        line.push_back(vector_to_string(pot_));
                        return line;
                }

        private:
                std::shared_ptr<Computer> comp_;
                std::vector<GEdge const*> path_;
                std::vector<size_t> active_;
                Eigen::VectorXd pot_;
        };
        struct IndexMaker : IndexMakerConcept{
                explicit IndexMaker(GameTree const& S){
                        for(auto d : S){
                                for(auto e : d){
                                        D[e] = d.GetIndex();
                                        P[e] = d.GetPlayer();
                                        I[e] = d.OffsetFor(e);
                                }
                        }
        
                }
                virtual std::vector<Index> MakeIndex(std::vector<GEdge const*> const& path, holdem_class_vector const& cv)const override{
                        std::vector<Index> index;
                        for(auto e : path){
                                index.push_back( Index{ D.Color(e), I.Color(e), cv[P.Color(e)] } );
                        }
                        return index;
                }
        private:
                GraphColouring<size_t> D;
                GraphColouring<size_t> P;
                GraphColouring<size_t> I;
        };


        inline
        GraphColouring<AggregateComputer> MakeComputer(std::shared_ptr<GameTree> gt){
                boost::timer::cpu_timer timer;
                timer.start();
                PS_LOG(trace) << "Making computer for " << gt->StringDescription() << "...";
                auto root   = gt->Root();
                auto state0 = gt->InitialState();

                BOOST_SCOPE_EXIT_ALL(&){
                        PS_LOG(trace) << "Making computer took " << timer.format();
                };


                
                /*
                 * To simplify the construction, we first create a object which represents
                 * the teriminal state of the game, for hu we have 3 terminal states
                 *                        {f,pf,pp},
                 * with the first two being independent of the dealt hand, whilst the 
                 * last state pp required all in equity evaluation. We create a vector
                 * of each of these terminal states, then which create Computer objects
                 * for fast computation. This is mainly to simplify the code.
                 */
                std::vector<std::shared_ptr<MakerConcept> > maker_dev;

                /*
                 * Now we go over all the terminal nodes, which is basically a path to
                 * something that can be evaluated. 
                 */
                GraphColouring<AggregateComputer> AG;
                GraphColouring<PushFoldOperator> ops;
                gt->ColorOperators(ops);

                for(auto t : root->TerminalNodes() ){

                        auto path = t->EdgePath();
                        auto ptr = std::make_shared<Computer>(path_to_string(path));
                        AG[t].push_back(ptr);
                        for(auto e : path ){
                                AG[e->From()].push_back(ptr); 
                        }

                        /*
                         * Work out the terminal state, independent of the deal, and
                         * make an factory object
                         */
                        auto state = state0;
                        for( auto e : path ){
                                ops[e](state);
                        }
                        std::vector<size_t> perm(state.Active.begin(), state.Active.end());
                        maker_dev.push_back(std::make_shared<StaticEmit>(ptr, path, perm, state.Pot) );

                }
                
                std::string cache_name{".cc.bin.prod"};
                class_cache C;
                C.load(cache_name);
                IndexMaker im(*gt);
                        
                MakerConcept::Display(maker_dev);

                auto nv = [&](double prob, holdem_class_vector const& cv){
                        for(auto& m : maker_dev ){
                                m->Emit(&im, &C, prob, cv );
                        }
                };
                gt->VisitProbabilityDist(nv);

                return AG;
        }
        /*
                Perfect example of where something is a candidate to be
                laxyilty initalixed. Creating the computer takes significant
                time (several minutes), and we don't need it if the solution
                already exists in the cache, but at the same time want the 
                cache to be transparebnt.

         */
        struct LazyComputer{
                struct Impl{
                        explicit Impl(std::shared_ptr<GameTree> game_tree_)
                                :game_tree(game_tree_)
                        {}
                        std::shared_ptr<GameTree> game_tree;
                        boost::optional<GraphColouring<AggregateComputer> > computer;
                };
                explicit LazyComputer(std::shared_ptr<GameTree> gt)
                        :impl_{std::make_shared<Impl>(gt)}
                {}
                // const we we have pass as const& 
                GraphColouring<AggregateComputer>& Get()const{
                        assert( impl_ && "precondition failed" );
                        if( ! impl_->computer ){
                                impl_->computer = MakeComputer(impl_->game_tree);
                        }
                        return impl_->computer.get();
                }
        private:
                mutable std::shared_ptr<Impl> impl_;
        };

} // end namespace sim
} // end namespace ps

#endif // PS_SIM_COMPUTER_FACTORY_H
