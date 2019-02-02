#ifndef PS_SIM_GAME_TREE_H
#define PS_SIM_GAME_TREE_H

#include "ps/detail/graph.h"

namespace ps{
namespace sim{
        using detail::GNode;
        using detail::GEdge;
        using detail::Graph;
        using detail::GraphColouring;



        // this captures the current game state
        struct PushFoldState{
                std::set<size_t> Active;
                Eigen::VectorXd Pot;
                Eigen::VectorXd Stacks;
                void Display()const{
                        std::cout << "detail::to_string(Active) => " << detail::to_string(Active) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(Active))
                        std::cout << "vector_to_string(Pot) => " << vector_to_string(Pot) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(Pot))
                        std::cout << "vector_to_string(Stacks) => " << vector_to_string(Stacks) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(Stacks))
                }
        };


        // These are operators which act on the player states

        struct Fold{
                size_t player_idx;
                void operator()(PushFoldState& S)const{
                        S.Active.erase(player_idx);
                }
        };
        struct Push{
                size_t player_idx;
                void operator()(PushFoldState& S)const{
                        auto x = S.Stacks[player_idx];
                        S.Stacks[player_idx] = 0.0;
                        S.Pot[player_idx] += x;
                }
        };
        struct Raise{
                size_t player_idx;
                double amt;
                void operator()(PushFoldState& S)const{
                        S.Stacks[player_idx] -= amt;
                        S.Pot[player_idx] += amt;
                }
        };
        using PushFoldOperator = std::function<void(PushFoldState&)>;



        struct Decision{
                Decision(size_t ID, std::string const& action, size_t player_idx)
                        :ID_(ID),
                        action_{action},
                        player_idx_(player_idx)
                {}
                friend std::ostream& operator<<(std::ostream& ostr, Decision const& self){
                        ostr << "ID_ = " << self.ID_;
                        ostr << ", player_idx_ = " << self.player_idx_;
                        ostr << ", CommonRoot() -> " << self.CommonRoot()->Name();
                        return ostr;
                }
                void Add(GEdge* e){ E.push_back(e); }
                size_t OffsetFor(GEdge const* e)const{
                        auto begin = E.begin();
                        auto end = E.end();
                        auto iter = std::find(begin, end, e);
                        if( iter == end)
                                BOOST_THROW_EXCEPTION(std::domain_error("not an edge"));
                        return static_cast<size_t>(std::distance(begin, iter));
                }
                Index IndexFor(GEdge const* e, holdem_class_id cid)const{
                        return { ID_, OffsetFor(e), cid};
                }

                size_t GetIndex()const{ return ID_; }
                size_t GetPlayer()const{ return player_idx_; }

                auto begin()const{ return E.begin(); }
                auto end()const{ return E.end(); }

                size_t size()const{ return E.size(); }

                auto const& Edges()const{ return E; }

                GNode* CommonRoot()const{
                        // should all be the same 
                        return E.back()->From();
                }

                std::string const& PrettyAction()const{ return action_; }
                
        private:
                size_t ID_;
                std::string action_;
                size_t player_idx_;
                std::vector<GEdge*> E;
        };

        struct GameTree{
                virtual ~GameTree()=default;
                /*
                 * We need to color each non-terminal node which which player's
                 * turn it is
                 */
                virtual void ColorPlayers(GraphColouring<size_t>& P)const=0;
                /*
                 * We color each node with an operator, this is how we figure 
                 * out what state we will be in for the terminal
                 */
                virtual void ColorOperators(GraphColouring<PushFoldOperator>& ops)const=0;
                
                virtual void ColorPrettyActions(GraphColouring<std::string>& A)const=0;
                /*
                 * We create the inital state of the game, this is used 
                 * for evaulting terminal nodes
                 */
                virtual PushFoldState InitialState()const=0;
                virtual GNode* Root()=0;
                virtual size_t NumPlayers()const=0;

                virtual std::string StringDescription()const=0;

                /*
                 * We have this because we need to visit every permutation of the form
                 *                  ( (P(cv_0),cv_0), (P(cv_1),cv_1), ... )
                 * which that 
                 *                  \sigma_n P(cv_n) = 1.
                 * As an implementation detail, we cache this in memory, so we have this
                 * function
                 */
                virtual void VisitProbabilityDist(std::function<void(double, holdem_class_vector const& cv)> const& V)const noexcept=0;


                using decision_vector = std::vector<std::shared_ptr<Decision> >;
                using decision_iterator = boost::indirect_iterator<decision_vector::const_iterator>;
                decision_iterator begin()const{ return v_.begin(); }
                decision_iterator end()const{ return v_.end(); }
                void Add(std::shared_ptr<Decision> ptr){
                        v_.push_back(ptr);
                }
                size_t NumDecisions()const{ return v_.size(); }
                size_t Depth(size_t j)const{ return v_[j]->size(); }

                StateType MakeDefaultState()const{
                        std::vector<std::vector<Eigen::VectorXd> > S;
                        for(auto const& d : *this){
                                Eigen::VectorXd proto(169);
                                proto.fill(1.0 / d.size());
                                S.emplace_back(d.size(), proto);
                        }
                        return S;
                }
                void Display()const{
                        for(auto d : v_){
                                std::cout << *d << "\n";
                        }
                }


                virtual double SmallBlind()const=0;
                virtual double BigBlind()const=0;
                // this is always helpfull
                virtual double EffectiveStack()const=0;
        
        protected:
                void BuildStrategy(){
                        /*
                         * We not iterate over the graph, and for each non-terminal node
                         * we allocate a decision to that node. This Decision then
                         * takes the player index, and strategy index.
                         */
                        GraphColouring<size_t> P;
                        GraphColouring<std::string> A;
                        ColorPlayers(P);
                        ColorPrettyActions(A);
                        std::vector<GNode*> stack{Root()};
                        size_t dix = 0;
                        for(;stack.size();){
                                auto head = stack.back();
                                stack.pop_back();
                                if( head->IsTerminal() )
                                        continue;
                                auto d = std::make_shared<Decision>(dix++, A[head], P[head]);
                                this->Add(d);
                                for( auto e : head->OutEdges() ){
                                        d->Add(e);
                                        stack.push_back(e->To());
                                }
                        }
                }
        private:
                std::vector<std::shared_ptr<Decision> > v_;
        };




        

} // end namespace sim
} // end namespace ps


#include "ps/sim/game_tree_impl.h"


#endif // PS_SIM_GAME_TREE_H
