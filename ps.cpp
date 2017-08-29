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

// This class contains all the state and history or a hand
//
// This contains NO game logic
struct poker_hand{
        double sb();
        double bb();
        double btn();

        struct player_type{
                double stack();
                size_t idx();
                std::string name();
        };

        player_type const& player(size_t idx); 
};

/*
        Contains the inital state tail_, the current state
        head_, and all the sequence of actiions in the 
        form
                head_ = f4(f3(f2(f1(f0(tail_))))),
        etc
 */
struct hand_history{
private:
        poker_hand tail_;
        poker_hand head_;
        std::vector<action_t> actions_;
};

struct player_ctrl{
        virtual void fold();
        virtual void raise(double amt);
        virtual void push();
};

struct hash_ctrl : player_ctrl{
        void fold(){ hash_ += "f"; }
        void raise(double amt){ hash_ += "r"; }
        void push(){ hash_ += "p"; }
        std::string const& get()const{ return hash_; }
private:
        std::string hash_;
};

struct player_strategy{
        virtual action_t play_turn(size_t idx, hand_history const& hh, holdem_id hand )=0;
};
struct simple_player_strategy : player_strategy{
        action_t play_turn(size_t idx, hand_history const& hh, holdem_id hand)override{
                hash_ctrl hasher;
                hand.replay( hasher );
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
struct any_hand_model;


/*
        This assigns a discrete probabilty distribution
        to the various hands.
*/
struct any_hand_model_strategy{
};

void any_hand_model_strategy_test(){
        any_hand_model_strategy strat;
        
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

struct simple_player_strategy : player_strategy{
        action_t play_turn(size_t idx,
                           hand_history const& hh,
                           weighted_range const& rng,
                           player_ctrl& ctrl)override{
                hash_ctrl hasher;
                hand.replay( hasher );
                if( hasher.get() == "" ){
                        auto 

                } else if( hasher.get() == "p"){
                        return push_;
                } else{
                        return fold_;
                }
                return default_;
        }
};

struct player_set{
private:
        std::vector<std::shared_ptr<player_strategy> strat_;
};

struct game_logic{
        virtual void execute_hand(poker_hand const& prototype, 
                                  player_set const& players)=0;
};

struct push_fold_game_logic{
        virtual void execute_hand(poker_hand const& prototype, 
                                  player_set const& players)override
        {
        }
};

struct game_tree_node{
        std::unique_ptr<game_logic>  head_;
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




int main(){
}


