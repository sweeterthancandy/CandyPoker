#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown.h"
#include "ps/sim/holdem_class_strategy.h"

#if 1

/*
                Strategy in the form 
                        vpip/fold
 */

using namespace ps;

struct player_decl{
        virtual hand_vector get_hv()const;
        auto begin()const{ return get_hv().begin(); }
        auto end()const{ return get_hv().end(); }
};
struct player_range : player_decl{};
struct placeholder : player_decl{};

struct simulation_decl{
        std::vector<std::shared_ptr<player_decl> > player_;
        std::vector<double> stacks_;
        double sb_;
        double bb_;
};

struct simulation_breakdown{
};

enum player_action{
        PlayerAction_Push,
        PlayerAction_Fold
};

/*
 
 */
struct context{
        double sb_;
        double bb_;
        std::vector<double> stack_;
};


struct node{
        virtual void eval(context& ctx)const;
}:
struct push : node{
};
struct fold : node{
};

struct choice : node{
private:
        push push_;
        fold fold_;
};

struct strategy{
        virtual player_action act(context& ctx,
                                  hand_history const& hist, // what happened so far
                                  holdem_hand_id hand )=0;
};


/*
                2 players

                             P1         P2
                        -- (fold) -->                 f
                        -- (push) --> (fold) -->      pf
                        -- (push) --> (call) -->      pp
                             
                3 players
                             P1         P2         P3
                        -- (fold) --> (fold)              ff
                        -- (fold) --> (push) --> (fold)   fpf
                        -- (fold) --> (push) --> (call)   fpp
                        -- (push) --> (fold) --> (fold)   pff
                        -- (push) --> (fold) --> (call)   pfp
                        -- (push) --> (call) --> (fold)   ppf
                        -- (push) --> (call) --> (call)   ppp
 */
std::shared_ptr<simulation_breakdown> run_simulation( simulation_decl const& decl)
{
}
#endif

int main(){
}
