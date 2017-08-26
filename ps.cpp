#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <bitset>
#include <cstdint>
#include <future>
#include <boost/format.hpp>
#include <boost/timer/timer.hpp>

#include "ps/base/cards.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/sim/holdem_class_strategy.h"

#include <boost/range/algorithm.hpp>
#include <boost/format.hpp>
#include <random>

/*
        
                                <start>
                                   |
                                <deal>
                                   |
                          <while active players>
                                        <get player action>
                                   |
                                <end>
                                  
                

 */


using namespace ps;


// this describes the setup of the game (before any action)
// ie we can construct a game from the game_decl
struct game_decl{
        game_decl(double sb, double bb)
                :sb_{sb},bb_{bb}
        {}
        void push_stack(double amt){
                stacks_.push_back(amt);
        }
        auto get_sb()const{ return sb_; }
        auto get_bb()const{ return bb_; }
        std::vector<double> const& get_stacks()const{ return stacks_; }
        auto players_size()const{
                return stacks_.size();
        }
private:
        double sb_;
        double bb_;
        double ante_{0.0};
        std::vector<double> stacks_;
};

enum PlayerAction{
        PlayerAction_PostSB,
        PlayerAction_PostSBAllin,
        PlayerAction_PostBB,
        PlayerAction_PostBBAllin,
        PlayerAction_Fold,
        PlayerAction_Push
};

// we need to be able see what happened
struct action_decl{
        // idea here is that offset can be deduced later
        explicit action_decl(PlayerAction action, size_t offset = -1 )
                :offset_{offset}
                ,action_{action}
        {
        }
        void set_offset(size_t _offset){ offset_ = _offset; }
        auto offset()const{return offset_;}
        // idea here is that we Post Sb, but it might be a SB_Allin 
        void set_action(PlayerAction _action){ action_ = _action;}
        auto action()const{return action_;}
private:
        size_t offset_;
        PlayerAction action_;
};




enum PlayerState{
        PlayerState_Active,
        PlayerState_Fold,
        PlayerState_AllIn
};
std::string PlayerState_to_string(PlayerState e) {
        switch (e) {
        case PlayerState_Active:
                return "PlayerState_Active";
        case PlayerState_Fold:
                return "PlayerState_Fold";
        case PlayerState_AllIn:
                return "PlayerState_AllIn";
        default:
                return "__unknown__";
        }
}

// This holds the current state of the game
struct game_context{

        struct player_context{
                player_context(double stack)
                        :state_{PlayerState_Active}
                        ,starting_stack_{stack}
                        ,stack_{stack}
                {}
                auto state()const         { return state_; }
                auto starting_stack()const{ return starting_stack_; }
                auto stack()const         { return stack_; }
                friend std::ostream& operator<<(std::ostream& ostr, player_context const& self){
                        return ostr << "{" << PlayerState_to_string(self.state()) << "," << self.stack() << "}";
                }
        private:
                friend struct game_context;
                PlayerState state_;
                double starting_stack_;
                double stack_;
        };

        game_context(game_decl const& decl, size_t btn)
                :btn_{btn}
                ,decl_{decl}
                ,active_count_{decl_.players_size()}
        {
                cursor_  = ( decl_.players_size() == 2 ? 0 : 1 );
                for(size_t i=0; i!= decl.players_size();++i){
                        players_.emplace_back( decl.get_stacks()[i] );
                }
        }
        player_context const& get_player(size_t btn_offset)const{
                size_t idx = ( btn_offset + btn_ ) % decl_.players_size();
                return players_[idx];
        }
        player_context      & get_player(size_t btn_offset){
                return const_cast<player_context&>(
                        reinterpret_cast<game_context const*>(this)
                                ->get_player(btn_offset)
                        );
        }
        auto btn()const          { return btn_; }
        auto active_count()const{ return active_count_; }
        auto allin_count()const { return allin_count_; }
        std::vector<action_decl> const& get_history()const{ return history_; }
        game_decl const& get_decl()const{ return decl_; }
        
        void post(PlayerAction pa){
                action_decl act(pa,  (cursor_ + decl_.players_size() - btn_ )%  decl_.players_size());

                // preprocess
                switch(act.action()){
                case PlayerAction_PostSB:
                        if( players_[cursor_].stack_ <= decl_.get_sb() ){
                                act.set_action(PlayerAction_PostSBAllin);
                        }
                        break;
                case PlayerAction_PostBB:
                        if( players_[cursor_].stack_ <= decl_.get_bb() ){
                                act.set_action(PlayerAction_PostBBAllin);
                        }
                        break;
                default:
                        break;
                }


                // apply action
                switch(act.action()){
                case PlayerAction_PostSB:
                        post_pot_( decl_.get_sb() );
                        players_[cursor_].stack_ -= decl_.get_sb();
                        break;
                case PlayerAction_PostSBAllin:
                        post_pot_( players_[cursor_].stack_ );
                        players_[cursor_].state_ = PlayerState_AllIn;
                        players_[cursor_].stack_ = 0.0; 
                        ++allin_count_;
                        --active_count_;
                        break;
                case PlayerAction_PostBB:
                        post_pot_( decl_.get_bb() );
                        players_[cursor_].stack_ -= decl_.get_bb();
                        break;
                case PlayerAction_PostBBAllin:
                        post_pot_( players_[cursor_].stack_ );
                        players_[cursor_].state_ = PlayerState_AllIn;
                        players_[cursor_].stack_ = 0.0; 
                        ++allin_count_;
                        --active_count_;
                        break;
                case PlayerAction_Push:
                        post_pot_( players_[cursor_].stack_ );
                        players_[cursor_].state_ = PlayerState_AllIn;
                        players_[cursor_].stack_ = 0.0; 
                        ++allin_count_;
                        --active_count_;
                        break;
                case PlayerAction_Fold:
                        players_[cursor_].state_ = PlayerState_Fold;
                        --active_count_;
                        break;
                }
                history_.push_back(std::move(act));
                next_();
        }
        void display(std::ostream& ostr = std::cout)const{
                for(size_t i=1;i<=decl_.players_size();++i){
                        size_t idx = ( i + btn_ ) % decl_.players_size();
                        size_t offset = ( i %        decl_.players_size() );
                        ostr << str(boost::format("    %-5s - %s") % format_pos_(offset) % players_[offset]) << "\n";
                }
                std::cout << std::string(20,'-') << format_state_() << std::string(20,'-') << "\n";
        }
        friend std::ostream& operator<<(std::ostream& ostr, game_context const& self){
                self.display(ostr);
                return ostr;
        }
private:
        void post_pot_(double amt){
                pot_ += amt;
        }
        std::string format_state_()const{
                std::stringstream sstr;
                sstr << " pot " << pot_ << " - " << ( end_flag_ ? "end" : "running" );
                sstr << ", ";
                STREAM_SEQ(sstr, (active_count_)(allin_count_));
                return sstr.str();
        }
        std::string format_pos_(size_t offset)const{
                switch(offset){
                case 0:
                        return "BTN";
                case 1:
                        return "SB";
                case 2:
                        return "BB";
                default:
                        break;
                }
                auto rev_offset = decl_.players_size() - offset;
                #if 0
                if( rev_offset < offset ){
                } else {
                }
                #endif
                return str(boost::format("BTN+%d") % rev_offset);

        }
        void next_(){
                if( ( allin_count_ == 0 && active_count_ == 1 ) ||
                    active_count_ == 0  ){
                        end_flag_ = true;
                        return;
                }
                for(;;){
                        ++cursor_;
                        cursor_ = (cursor_ % decl_.players_size());
                        if( players_[cursor_].state_ == PlayerState_Active )
                                break;
                }
        }

        size_t btn_;
        // this is the player who's action is on
        size_t cursor_;
        game_decl decl_;
        size_t active_count_;
        size_t allin_count_{0};
        std::vector<player_context> players_;
        std::vector<action_decl> history_;
        bool end_flag_{false};
        double pot_{0.0};
};

// {{{
#if 0
struct player_strat{
        virtual ~player_strat()=default;
        PlayerAction act(game_context const& ctx)=0;
};

// this has all the players strategyies
struct players{
        std::vector<std::shared_ptr<player_strat> > vec_;
};



// 
struct simultation_context{
        simultation_context(game_decl const& decl, players const& p)
                :n_{ctx_.size()}
                ,decl_{decl}
                ,p_{p}
        {

        }
        void simulate(){
                std::default_random_engine gen_;
                std::uniform_real_distribution<double> zero_one_dist{.0,1.0};
                std::uniform_real_distribution<double> button_dist{0,n_-1};
                
                dealer d;
                
                auto btn = button_dist(gen_);
                auto offset = ( n_ == 2 ? btn : btn + 3 );

                game_context ctx(decl_, btn);

                game_enginer engine(ctx, btn);

                engine.post_sb();
                engine.post_bb();

                for(size_t cursor =0; cursor != n_;++cursor){
                        auto hand = d.deal();

                        auto c = p_.vec_[cursor + offset]->act( ctx );
                        switch(c){
                        case Choice_Fold:
                                engine.fold();
                                break;
                        case Choice_Push:
                                engine.push();
                                break;
                        }

                        if( engine.no_more_action() ){
                                break;
                        }
                }
        }
private:
        size_t n_;
        game_decl decl_;
        players p_;
};

void run_simulation_test(){
        game_decl decl(0.5, 1);
        decl.push_player(10);
        decl.push_player(10);

        players p;
        p.push_strat(std::make_shared<pf_strat>(holdem_class_strategy(1.0)));
        p.push_strat(std::make_shared<pf_strat>(holdem_class_strategy(1.0)));

        simultation_context sim(decl, p);

        for(size_t i=0;i!=10000;++i)
                sim.simulate();

        auto result = sim.result();
}
#endif
/// }}}


int main(){
        game_decl decl(0.5, 1.0);
        decl.push_stack(10);
        decl.push_stack(10);
        decl.push_stack(10);
        game_context ctx(decl, 0);
        ctx.display();
        ctx.post(PlayerAction_PostSB);
        ctx.post(PlayerAction_PostBB);
        ctx.display();
        ctx.post(PlayerAction_Fold);
        ctx.display();
        ctx.post(PlayerAction_Push);
        ctx.post(PlayerAction_Push);
        ctx.display();
}
