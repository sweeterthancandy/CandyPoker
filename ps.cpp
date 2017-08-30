#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <bitset>
#include <cstdint>
#include <future>
#include <boost/format.hpp>
#include <boost/timer/timer.hpp>

#include "ps/support/config.h"
#include "ps/support/index_sequence.h"
#include "ps/base/cards.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/sim/holdem_class_strategy.h"

#include <boost/range/algorithm.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>

#include <random>



using namespace ps;

struct action_t{
        enum Type{
                Type_Unknown,
                Type_PostSB,
                Type_PostBB,
                Type_Fold,
                Type_Push
        };
        action_t():type_{Type_Unknown}{}
        explicit action_t(size_t idx, Type type, size_t double val = 0.):
                type_{type},idx_{idx}, value_{val}
        {}
        auto type()const{ return type_; }
        auto idx()const{ return idx_; }
private:
        Type type_;
        size_t idx_;
        double value_; // only for type \in {Push, Raise}
};

// This class contains all the state and history or a hand
//
// This contains NO game logic
// this describes the setup of the game (before any action)
struct hand_decl
{
        struct player_decl{
                player_decl(double stack, std::string name)
                        :stack_{stack}
                        ,name_{std::move(name)}
                {}
                auto stack()const{ return stack_; }
                auto const& name()const{ return name_; }
        private:
                double stack_;
                std::string name_;
        };

        hand_decl(double sb, double bb)
                :sb_{sb},bb_{bb}
        {}
        void push_player(double amt){
                push_player(amt, "player_" + boost::lexical_cast<std::string>(players_.size()));
        }
        void push_player(double amt, std::string name){
                players_.emplace_back(amt, std::move(name));
        }
        auto get_sb()const{ return sb_; }
        auto get_bb()const{ return bb_; }
        auto players_size()const{
                return players_.size();
        }
        auto const& player(size_t idx)const{
                return players_.at(idx);
        }
        auto begin()const{ return players_.begin(); }
        auto end()const  { return players_.end(); }
private:
        double sb_;
        double bb_;
        //double ante_{0.0};
        std::vector<player_decl> players_;
};

enum PlayerState{
        PlayerState_Active,
        PlayerState_Fold,
        PlayerState_AllIn
};

// Idea here is that we're going to be getting alot of copying
struct hand_state{
        struct lightweight_player{
                double stack;
                PlayerState state;
        };
        lightweight_player      & operator[](size_t idx)     { return players_.at(idx); }
        lightweight_player const& operator[](size_t idx)const{ return players_.at(idx); }
        size_t pot = 0;
private:
        std::vector<lightweight_player> players_;
};
/*
   This reorientes the board w.r.t. to the active
   player or otherwise
 */

/*
        Contains the inital state tail_, the current state
        head_, and all the sequence of actiions in the 
        form
                head_ = f4(f3(f2(f1(f0(tail_))))),
        etc
 */
struct hand_history{
        auto const& deal()const{ return decl_; }
        auto const& head()const{ return head_; }
        auto&       head()     { return head_; }
        void post(action_t action){
                // XXX this looks like part of the game logic to me
                switch(action.type()){
                case action_t::Type_PostSB:
                case action_t::Type_PostBB:{
                        auto blind = action.type() == action_t::Type_PostSB ? decl_.sb() : decl_.bb();
                        if( blind >= head_[action.idx()].stack){
                                head_.pot += head_[action.idx()].stack;
                                head_[action.idx()].stack = 0;
                                head_[action.idx()].state = PlayerState_AllIn;
                        } else{
                                head_.pot += blind;
                                head_[action.idx()].stack -= blind;
                        }
                }
                        break;
                case action_t::Type_Fold:
                        head_[action.idx()].state = PlayerState_Fold;
                        break;
                case action_t::Type_Push:
                        head_.pot += head_[action.idx()].stack;
                        head_[action.idx()].stack = 0;
                        head_[action.idx()].state = PlayerState_AllIn;
                        break;
                default:
                        PS_UNREACHABLE();
                }
                actions_.push_back(std::move(action));
        }
        auto begin()const{ return actions_.begin(); }
        auto end()  const{ return actions_.end();   }
private:
        hand_decl decl_;
        hand_state head_;
        std::vector<action_t> actions_;
};


struct player_strategy{
        virtual action_t play_turn(size_t idx, hand_history const& hh, holdem_id hand )=0;
};
struct simple_player_strategy : player_strategy
{
        any_hand_model_strategy play_turn(any_hand_model_strategy& strat, size_t idx, hand_history const& hh)override{
                hash_ctrl hasher;
                hand.replay( hasher );
        
                 strat;
                if( hasher.get() == "" ){
                        return push_;
                } else if( hasher.get() == "p"){
                        return push_;
                } else{
                        return fold_;
                }
                return default_;
        }
};

/*
        holdem_id       => specific hand
        holdem_class_id => specific class of hands
        holdem_range    => range of hands
        weighted_range  => weight range
 */
struct any_hand_model{
        enum Model{
                Model_Hand,
                Model_Class,
                Model_ClassRange
        };
};


/*
        This assigns a discrete probabilty distribution
        to the various hands.
*/

struct class_strategy{

        using this_t = class_strategy;

        struct distribution_set_proxy{
                distribution_set_proxy(this_t* _this,
                                       holdem_class_vector vec)
                        :this_{_this}
                        ,vec_{std::move(vec)}
                {}
                void push(){
                        assign_(_push);
                        destory_();
                }
                void fold(){
                        assign_(_fold);
                        destory_();
                }
        private:
                void assign_(action const& proto){
                        for( auto id : vec_){
                                this_->strat_[id] = proto;
                        }
                }
                // make sure can only be set once
                void destory_(){
                        this_ = nullptr;
                }
                this_t this_;
                holdem_class_vector vec_;
        };
        distribution_set_proxy range(holdem_class_vector vec){
                return distribution_set_proxy{this, std::move(vec)};
        }
        distribution_set_proxy default_(){
                holdem_class_vector vec;
                for( size_t i=0;i!=strat_.size();++i){
                        if( strat_.type() == action::Type_Unknown){
                                vec.push_back(i);
                        }
                }
                return range(std::move(vec));
        }
        distribution_set_proxy operator[](holdem_class_id id){
                holdem_class_vector vec;
                vec.push_back(id);
                return range(std::move(vec));
        }
private:
        std::array< action, 169 > strat_;
};

#if 0
void any_hand_model_strategy_test(){
        
        strats.default_()
                .fold();

        for( auto& item : strat){
                item.fold();
        }

        strats.range(_AA - _TT )
                .push();
        strats.range(_AA - _TT )
                .prob(.1).push()
                .prob(.2).call()
                .deafult_().fold();
                        
}
#endif


struct player_set{
private:
        std::vector<std::shared_ptr<player_strategy> strat_;
};

struct game_logic{
        enum State{
                State_Running,
                State_Muck,
                State_Showdown
        };
        virtual State push()=0;
        virtual State fold()=0;
};

struct game_logic_

struct game_tree_node{
        poker_hand hand_;
        size_t player_id_;
        std::vector<game_tree_node> tail_;
};

struct push_fold_logic{
        push_fold_logic(poker_hand const& hand){
        }
        std::vector<push_fold_logic> yeild(player_set const& ps)const{
        }
private:
        any_range range_;
        action_t action_;
        poker_hand hand_;
};

struct push_fold_game_tree{
        void execute_hand(poker_hand const& prototype, 
                          player_set const& players)override
        {
                game_tree_node root_;
                root_.head_ = std::make_unique<push_fold_logic>(prototype);

        std::vector<game_tree_node*> stack;
        stack.push_back(&root_);

        for(;stack.size();){
                auto ptr = stack.back();
                stack.pop_back();

                auto const& head = (**ptr).head_;
                auto strat  = head.yield(players);

                for( auto const& p : strat ){
                        auto branch = head;
                        branch.apply( p );
                        (**ptr).tail_.push_back(std::move(branch));
                }
                for( auto& ref : (**ptr).tail_ ){
                        if( ! ref.is_terminal() ){
                                stack.push_back(&ref);
                        }
                }
        }
}
};


struct hand_driver{
        explicit hand_driver( poker_hand const& init)
                :hh_{init}
                ,n_{hh_.head().players_size()}
                ,active_{n_,true}
                ,allin_{n_,false}
                ,active_count_{n_}
        {
                if( n_ == 2 ){
                        active_idx_ = hh_.head().btn();
                } else{
                        active_idx_ = ( hh_.head().btn() + 2 ) % n_;
                }
        }
        void run(){
                hh_.post( action_t{ active_idx_, action_t::Type_PostSB});
                next_();
                hh_.post( action_t{ active_idx_, action_t::Type_PostBB});
                next_();
        }
private:
        void next_(){
                for(;;){
                        ++active_idx_;
                        active_idx_ = active_ % n_;
                        if( active_[active_idx_] )
                                break;
                }
        }
        hand_history hh_;
        size_t n_;
        size_t active_count_;
        std::vector<bool> active_;
        std::vector<bool> allin_;
        size_t active_idx_;
}

struct node{

        hand_history hh_;
        node* push_;
        node* fold_;
};




int main(){
}


