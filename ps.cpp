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
#include "ps/base/cards.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/sim/holdem_class_strategy.h"

#include <boost/range/algorithm.hpp>
#include <boost/format.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>

#include <random>



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
        auto stack(size_t idx)const{ return stacks_[idx]; }
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

std::string PlayerAction_to_string(PlayerAction e) {
        switch (e) {
        case PlayerAction_PostSB:
                return "PlayerAction_PostSB";
        case PlayerAction_PostSBAllin:
                return "PlayerAction_PostSBAllin";
        case PlayerAction_PostBB:
                return "PlayerAction_PostBB";
        case PlayerAction_PostBBAllin:
                return "PlayerAction_PostBBAllin";
        case PlayerAction_Fold:
                return "PlayerAction_Fold";
        case PlayerAction_Push:
                return "PlayerAction_Push";
        default:
                return "__unknown__";
        }
}
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
                ,n_{decl.players_size()}
                ,active_count_{n()}
        {
                assert( n() >= 2 && "precondition failed");


                if(  n() == 2 ){
                        // special HU case
                        sb_offset_ = btn;
                        cursor_ = btn;
                } else {
                        sb_offset_ = ( btn + 1 ) % n();
                        cursor_ =   (sb_offset_ + 2 ) % n();
                }
                for(size_t i=0; i!= decl.players_size();++i){
                        players_.emplace_back( decl.get_stacks()[i] );
                }
        }
        player_context const& get_player(size_t btn_offset)const{
                size_t idx = ( btn_offset + btn_ ) % n();
                return players_[idx];
        }
        player_context      & get_player(size_t btn_offset){
                return const_cast<player_context&>(
                        reinterpret_cast<game_context const*>(this)
                                ->get_player(btn_offset)
                        );
        }
        auto btn()const          { return btn_; }
        auto pot()const          { return pot_; }
        auto active_count()const{ return active_count_; }
        // this is the offset of players which is the action is on
        //
        // for example, then the button is on player 1, in a 3 player game,
        // at the start we have
        //       
        //       player | position
        //       -------+---------
        //            1 | btn
        //            2 | sb
        //            0 | bb
        // 
        // so that player 1 will be the active player, followed by 2,
        // followed by 0.
        //
        auto cursor()const{ return cursor_; }
        auto allin_count()const { return allin_count_; }
        auto players_size()const{ return n(); }
        std::vector<action_decl> const& get_history()const{ return history_; }
        game_decl const& get_decl()const{ return decl_; }
        size_t n()const{ return n_; }
        auto sb_offset()const{ return sb_offset_; }
        auto bb_offset()const{ return (sb_offset_+1) % n(); }
        

        void post_blinds(){
                post(PlayerAction_PostSB);
                if( ! players_left_to_act() )
                        return;
                post(PlayerAction_PostBB);
        }
        void post(PlayerAction pa){
                action_decl act(pa, cursor() );

                // preprocess
                switch(act.action()){
                case PlayerAction_PostSB:
                        if( players_[cursor()].stack_ <= decl_.get_sb() ){
                                act.set_action(PlayerAction_PostSBAllin);
                        }
                        break;
                case PlayerAction_PostBB:
                        if( players_[cursor()].stack_ <= decl_.get_bb() ){
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
                        players_[cursor()].stack_ -= decl_.get_sb();
                        break;
                case PlayerAction_PostSBAllin:
                        post_pot_( players_[cursor()].stack_ );
                        players_[cursor()].state_ = PlayerState_AllIn;
                        players_[cursor()].stack_ = 0.0; 
                        ++allin_count_;
                        --active_count_;
                        break;
                case PlayerAction_PostBB:
                        post_pot_( decl_.get_bb() );
                        players_[cursor()].stack_ -= decl_.get_bb();
                        break;
                case PlayerAction_PostBBAllin:
                        post_pot_( players_[cursor()].stack_ );
                        players_[cursor()].state_ = PlayerState_AllIn;
                        players_[cursor()].stack_ = 0.0; 
                        ++allin_count_;
                        --active_count_;
                        break;
                case PlayerAction_Push:
                        post_pot_( players_[cursor()].stack_ );
                        players_[cursor()].state_ = PlayerState_AllIn;
                        players_[cursor()].stack_ = 0.0; 
                        ++allin_count_;
                        --active_count_;
                        break;
                case PlayerAction_Fold:
                        players_[cursor()].state_ = PlayerState_Fold;
                        --active_count_;
                        break;
                }
                history_.push_back(std::move(act));
                next_();
        }
        void display(std::ostream& ostr = std::cout)const{
                size_t start = 1;
                size_t end = n() + 1;
                if( n() == 2 ){
                        start = 0;
                        end = n();
                }
                for(size_t i=start;i<end;++i){
                        size_t offset = ( btn_ + i) % n();
                        ostr << str(boost::format("    %-5s - %s") % format_pos_(i % n()) % players_[offset]) << "\n";
                }
                std::cout << std::string(10,'-') << format_state_() << std::string(10,'-') << "\n";
        }
        bool players_left_to_act()const{
                return ! end_flag_;
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
                sstr << " pot " << pot_;
                sstr << ", " << ( end_flag_ ? "end" : "running" );
                sstr << ", active_count_ " << active_count_;
                sstr << ", allin_count_ " << allin_count_;
                sstr << ", btn_ " << btn_;
                sstr << ", sb_offset " << sb_offset();
                sstr << ", bb_offset " << bb_offset();
                return sstr.str();
        }
        std::string format_pos_(size_t offset)const{
                if( n() == 2 ){
                        switch(offset){
                        case 0:
                                return "SB";
                        case 1:
                                return "BB";
                        default:
                                PS_UNREACHABLE();
                        }
                }
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
                auto rev_offset = n() - offset;
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
                        cursor_ = (cursor_ % n());
                        if( players_[cursor_].state_ == PlayerState_Active )
                                break;
                }
        }

        size_t btn_;
        // this is the player who's action is on
        size_t sb_offset_;
        size_t cursor_;
        game_decl decl_;
        size_t n_;
        size_t active_count_;
        size_t allin_count_{0};
        std::vector<player_context> players_;
        std::vector<action_decl> history_;
        bool end_flag_{false};
        double pot_{0.0};
};

// {{{
struct player_strat{
        virtual ~player_strat()=default;
        virtual PlayerAction act(game_context const& ctx, holdem_id id)=0;
};
struct push_player_strat : player_strat{
        PlayerAction act(game_context const& ctx, holdem_id id)override{
                return PlayerAction_Push;
        }
};
struct fold_player_strat : player_strat{
        PlayerAction act(game_context const& ctx, holdem_id id)override{
                return PlayerAction_Fold;
        }
};
/*
        This should be represented by a matrix


        n = 2

            start - push - push
                  |      \ fold
                  \ fold

        n = 3

            start - push - push - push
                  |      |      \ fold
                  |      \ fold
                  |
                  \ fold - push - push
                         |      \ fold
                         \ fold

 */
struct holdem_class_strat_player : player_strat{
        holdem_class_strat_player(holdem_class_strategy const& sb_strat,
                                  holdem_class_strategy const& bb_strat)
                :sb_strat_{sb_strat}
                ,bb_strat_{bb_strat}
        {}
        PlayerAction act(game_context const& ctx, holdem_id id)override{
                PlayerAction act = PlayerAction_Fold;
                auto class_ = holdem_hand_decl::get(id).class_();
                if(ctx.cursor() == ctx.sb_offset()){
                        // sb opening action
                        if( sb_strat_[class_] >= ctx.get_decl().get_stacks()[0]){
                                act =  PlayerAction_Push;
                        }
                } else{
                        // bb facing a push
                        if( bb_strat_[class_] >=ctx.get_decl().get_stacks()[0]){ 
                                act =  PlayerAction_Push;
                        }
                }
                //PRINT_SEQ((holdem_hand_decl::get(id))(PlayerAction_to_string(act)));
                return act;

        }
private:
        holdem_class_strategy sb_strat_;
        holdem_class_strategy bb_strat_;
};

namespace ba = boost::accumulators;

struct dealer{
        explicit dealer(size_t n)
                :n_{n}
                ,btn_dist_{0,n_-1}
        {}
        size_t shuffle_and_deal_btn(){
                removed_ = 0;
                auto btn = btn_dist_(gen_);
                btn_acc_(btn);
                #if 0
                std::cout << "BTN Mean:   " << ba::mean(btn_acc_) << std::endl;
                std::cout << "CRD Mean:   " << ba::mean(card_acc_) << std::endl;
                #endif
                return btn;
        }
        holdem_id deal(){
                auto x = deal_card_();
                auto y = deal_card_();
                auto id = holdem_hand_decl::make_id(x, y);
                //PRINT_SEQ((card_decl::get(x))(card_decl::get(y))(holdem_hand_decl::get(id)));
                card_acc_(id);
                return id;
        }
        size_t deck_size()const{
                return 52 - __builtin_popcount(removed_);
        }
private:
        card_id deal_card_(){
                for(;;){
                        auto cand = deck_(gen_);
                        auto mask = ( static_cast<size_t>(1) << cand);
                        if( !(removed_ & mask )){
                                removed_ |= mask;
                                return cand;
                        }
                }
        }
        size_t n_;
        //std::default_random_engine gen_;
        std::random_device gen_;
        std::uniform_int_distribution<card_id> deck_{0,52-1};
        std::uniform_int_distribution<size_t> btn_dist_;
        size_t removed_{0};

        ba::accumulator_set<double, ba::stats<ba::tag::mean >> btn_acc_;
        ba::accumulator_set<double, ba::stats<ba::tag::mean >> card_acc_;
};

/*
        This is the logic which processes how happens at the end of the hand,
        which players balances are credited/debits, and showdown is evaulated,
        or equity is evaulated
 */
struct game_evaluator{
        game_evaluator(){
                cev_ =  &equity_evaluator_factory::get("better");
                auto cache_ = &holdem_eval_cache_factory::get("main");
                auto class_cache_ = &holdem_class_eval_cache_factory::get("main");
                cev_->inject_cache( std::shared_ptr<holdem_eval_cache>(cache_, [](auto){}));
                cache_->load("cache_4.bin");
        }
        // returns a vector of the EV for the stacks
        std::vector<double> eval(game_context const& ctx, std::vector<holdem_id> const& deal){
                std::vector<double> d(ctx.players_size());
                
                do{
                        if(ctx.allin_count()==0){
                                // case walk
                                //PRINT("walk");
                                d[ctx.sb_offset()] -= ctx.get_decl().get_sb();
                                d[ctx.bb_offset()] += ctx.get_decl().get_sb();
                                break;
                        }
                        std::vector<size_t> p;
                        std::vector<holdem_id> aux;
                        for( size_t i=0; i!=ctx.players_size();++i){
                                if(ctx.get_player(i).state() == PlayerState_AllIn){
                                        p.push_back(i);
                                        aux.push_back(deal[i]);
                                }
                        }

                        if( p.size() == 1 ){
                                // case blind steal
                                //PRINT("blind steal");
                                d[ctx.sb_offset()] -= ctx.get_decl().get_sb();
                                d[p[0]]            += ctx.get_decl().get_sb();
                                d[ctx.bb_offset()] -= ctx.get_decl().get_bb();
                                d[p[0]]            += ctx.get_decl().get_bb();
                                break;
                        }
                        // case all in equity

                        auto equity = cev_->evaluate(aux);
                        for(size_t i=0;i!=p.size();++i){
                                d[p[i]] = ( ctx.pot() * equity->player(i).equity() ) - ctx.get_decl().stack(i);
                                //        \------equity in pot -----------/   \----- cost of bet -------------/
                        }
                        
                        #if 1
                        // need to deduced blinds if they arn't part of the allin
                        if( ctx.get_player(ctx.sb_offset()).state() != PlayerState_AllIn )
                                d[ctx.sb_offset()] -= ctx.get_decl().get_sb();
                        if( ctx.get_player(ctx.bb_offset()).state() != PlayerState_AllIn )
                                d[ctx.bb_offset()] -= ctx.get_decl().get_bb();
                                #endif
                }while(0);
                return std::move(d);
        }
private:
        equity_evaluator* cev_;
};

struct simultation_context{
        simultation_context(game_decl const& decl, std::vector<std::shared_ptr<player_strat> > const& p)
                :decl_{decl}
                ,p_{p}
                ,dealer_{decl_.players_size()}
        {
        }
        // returns vector of equity difference
        std::vector<double> simulate(){
                auto btn = dealer_.shuffle_and_deal_btn();
                game_context ctx(decl_, btn);

                ctx.post_blinds();

                holdem_hand_vector deal;
                deal.resize(ctx.n());
                for(size_t i=0;i!=deal.size(); ++i){
                        deal[i] = dealer_.deal();
                }

                for( ; ctx.players_left_to_act();){
                        #if 0
                        PRINT(ctx.cursor());
                        PRINT(deal[ctx.cursor()]);
                        PRINT( holdem_hand_decl::get(deal[ctx.cursor()]));
                        #endif
                        auto act = p_[ctx.cursor()]->act(ctx, deal[ctx.cursor()]);
                        ctx.post(act);
                }
                //ctx.display();

                auto ret = ge_.eval(ctx, deal);

                #if 0
                PRINT(deal);
                PRINT( detail::to_string(ret));
                #endif

                return std::move(ret);
        }
private:
        game_decl decl_;
        std::vector<std::shared_ptr<player_strat> > p_;
        dealer dealer_;
        game_evaluator ge_;
};

void run_simulation_test(){
        game_decl decl(0.5, 1);
        decl.push_stack(10);
        decl.push_stack(10);

        std::vector<std::shared_ptr<player_strat>> pv;

        holdem_class_strategy sb_strat;
        holdem_class_strategy bb_strat;

        sb_strat.load("sb_result.bin");
        bb_strat.load("bb_result.bin");

        sb_strat.display();
        bb_strat.display();

        #if 0
        auto pf_strat = std::make_shared<holdem_class_strat_player>(sb_strat, bb_strat);

        pv.push_back(pf_strat);
        pv.push_back(pf_strat);
        #endif
        #if 1
        pv.push_back(std::make_shared<fold_player_strat>());
        pv.push_back(std::make_shared<push_player_strat>());
        #endif

        simultation_context sim(decl, pv);


        std::vector<double> d(2);

        
        for(;;){
                for(size_t i=0;i!=10000;++i){
                        auto r =sim.simulate();
                        for(size_t j=0;j!=2;++j){
                                d[j] += r[j];
                        }
                }
                PRINT(detail::to_string(d));
        }


        //auto result = sim.result();
}
/// }}}

void game_context_test(){
        game_decl decl(0.5, 1.0);
        decl.push_stack(2);
        decl.push_stack(3);
        decl.push_stack(4);
        decl.push_stack(5);
        game_context ctx(decl, 3);
        ctx.display();
        ctx.post(PlayerAction_PostSB);
        ctx.post(PlayerAction_PostBB);
        ctx.post(PlayerAction_Fold);
        ctx.post(PlayerAction_Push);
        ctx.post(PlayerAction_Push);
        ctx.post(PlayerAction_Fold);
        ctx.display();


}
int main(){
        run_simulation_test();
}
