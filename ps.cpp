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
#include <boost/variant.hpp>

#include <random>



using namespace ps;



// this describes the setup of the game (before any action)
struct game_decl
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

        game_decl(double sb, double bb)
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

inline
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

enum PlayerAction{
        PlayerAction_PostSB,
        PlayerAction_PostSBAllin,
        PlayerAction_PostBB,
        PlayerAction_PostBBAllin,
        PlayerAction_Fold,
        PlayerAction_Push
};

inline
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


/* This desribes the game, from before the deal, all the past
 * history, and the current state
 */
struct hand_context{

        // XXX might want to save history here
        struct player_context{
                player_context(size_t idx, game_decl::player_decl const& proto)
                        :idx_{idx}
                        ,state_{PlayerState_Active}
                        ,name_{proto.name()}
                        ,starting_stack_{proto.stack()}
                        ,stack_{proto.stack()}
                {}
                auto        idx()           const{ return idx_;            }
                auto        state()         const{ return state_;          }
                auto&       state()              { return state_;          }
                auto        starting_stack()const{ return starting_stack_; }
                auto        stack()         const{ return stack_;          }
                auto&       stack()              { return stack_;          }
                auto const& name()          const{ return name_;           }
                auto        hand()          const{ return hand_;           }
                auto&       hand()               { return hand_;           }
                friend std::ostream& operator<<(std::ostream& ostr, player_context const& self){
                        std::stringstream tmp;
                        tmp << "{" 
                                << self.name()
                                << ", " << PlayerState_to_string(self.state()) 
                                << "," << self.stack() ;
                        if( self.hand_ != static_cast<holdem_id>(-1))
                                tmp << ", " << holdem_hand_decl::get(self.hand_);
                        tmp << "}";
                        return ostr << tmp.str();
                }
        private:
                friend struct hand_context;
                size_t idx_;
                PlayerState state_;
                std::string name_;
                double starting_stack_;
                double stack_;
                holdem_id hand_{static_cast<holdem_id>(-1)};
        };


        struct player_iterator{
                // psudo end iterator
                player_iterator():end_flag_{true}{}
                player_iterator(hand_context& ctx)
                        :ctx_{&ctx}
                {
                        if( ctx_->n() == 2){
                                // stats on the button
                                cursor_ = ctx_->btn();
                        } else {
                                // starts on the sb
                                // XXX this is a policy, not neccasarily
                                // intutive to start on the forced bets
                                cursor_ = ( ctx_->btn() + 1 ) % ctx_->n();
                        }
                }
                player_iterator operator++(){
                        if( ( ctx_->allin_count() == 0 && ctx_->active_count() == 1 ) ||
                              ctx_->active_count() == 0  ){
                                end_flag_ = true;
                        } else {
                                // find next active player
                                for(;;){
                                        ++cursor_;
                                        cursor_ = (cursor_ % ctx_->n());
                                        if( ctx_->player(cursor_).state() == PlayerState_Active )
                                                break;
                                }
                        }
                        return *this;
                }
                player_context& operator*(){ return ctx_->player(cursor_); } 
                player_context* operator->(){ return &ctx_->player(cursor_); } 
                player_context const& operator*()const{ return ctx_->player(cursor_); } 
                player_context const* operator->()const{ return &ctx_->player(cursor_); } 
                bool operator==(player_iterator const& that)const{
                        return end_flag_ == that.end_flag_;
                }
                bool operator!=(player_iterator const& that)const{
                        return end_flag_ != that.end_flag_;
                }
        private:
                size_t cursor_;
                hand_context* ctx_;
                bool end_flag_{false};
        };

        auto begin(){ return player_iterator{*this}; }
        auto end()  { return player_iterator{};      }

        hand_context(game_decl const& decl, size_t btn)
                :sb_{decl.get_sb()}
                ,bb_{decl.get_bb()}
                ,btn_{btn}
                ,n_{decl.players_size()}
                ,active_count_{n()}
        {
                assert( n() >= 2 && "precondition failed");

                if(  n() == 2 ){
                        sb_offset_ = btn;
                } else {
                        sb_offset_ = ( btn + 1 ) % n();
                }

                for(size_t i=0;i!=decl.players_size();++i){
                        players_.emplace_back(i, decl.player(i));
                }
        }
        player_context const& player(size_t idx)const{
                return players_[idx];
        }
        player_context      & player(size_t idx){
                return const_cast<player_context&>(
                        reinterpret_cast<hand_context const*>(this)
                                ->player(idx)
                        );
        }
        #if 0
        player_context const& player_from_btn(size_t btn_offset)const{
                size_t idx = ( btn_offset + btn_ ) % n();
                return player(idx);
        }
        player_context      & player_from_btn(size_t btn_offset){
                return const_cast<player_context&>(
                        reinterpret_cast<hand_context const*>(this)
                                ->player_from_btn(btn_offset)
                        );
        }
        #endif
        

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
        friend std::ostream& operator<<(std::ostream& ostr, hand_context const& self){
                self.display(ostr);
                return ostr;
        }

        double add_to_pot(double amt){
                return pot_ += amt;
        }
        
        size_t  btn()         const{ return btn_;                 }
        double  sb()          const{ return sb_;                  }
        double  bb()          const{ return bb_;                  }
        size_t  pot()         const{ return pot_;                 }
        size_t  active_count()const{ return active_count_;        }
        size_t& active_count()     { return active_count_;        }
        size_t  allin_count() const{ return allin_count_;         }
        size_t& allin_count()      { return allin_count_;         }
        size_t  players_size()const{ return n();                  }
        size_t  n()           const{ return n_;                   }
        size_t  sb_offset()   const{ return sb_offset_;           }
        size_t  bb_offset()   const{ return (sb_offset_+1) % n(); }
        size_t  utg()         const{ return (sb_offset_+2) % n(); }
private:
        std::string format_state_()const{
                std::stringstream sstr;
                sstr << " pot " << pot_;
                sstr << ", " << ( active_count_ == 0 ? "end" : "running" );
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
                return str(boost::format("BTN+%d") % rev_offset);

        }
        double sb_;
        double bb_;
        size_t btn_;
        size_t sb_offset_;
        size_t n_;
        size_t active_count_{0};
        size_t allin_count_{0};
        std::vector<player_context> players_;
        double pot_{0.0};
};

// the idea here, is the for a the most generic strategy implementation,
// I probably want to revisit the history
struct player_controller{
        virtual ~player_controller()=default;
        virtual void push         (hand_context& ctx, hand_context::player_iterator iter)=0;
        virtual void fold         (hand_context& ctx, hand_context::player_iterator iter)=0;
        virtual void post_sb      (hand_context& ctx, hand_context::player_iterator iter)=0;
        virtual void post_sb_allin(hand_context& ctx, hand_context::player_iterator iter)=0;
        virtual void post_bb      (hand_context& ctx, hand_context::player_iterator iter)=0;
        virtual void post_bb_allin(hand_context& ctx, hand_context::player_iterator iter)=0;
};

struct player_controller_default : player_controller
{
        void push(hand_context& ctx, hand_context::player_iterator iter)override{
                ctx.add_to_pot( iter->stack() );
                iter->state() = PlayerState_AllIn;
                iter->stack() = 0.0; 
                ++ctx.allin_count();
                --ctx.active_count();
        };
        void fold(hand_context& ctx, hand_context::player_iterator iter)override{
                iter->state() = PlayerState_Fold;
                --ctx.active_count();
        };
        void post_sb(hand_context& ctx, hand_context::player_iterator iter)override{
                ctx.add_to_pot( ctx.sb() );
                iter->stack() -= ctx.sb();
        };
        void post_sb_allin(hand_context& ctx, hand_context::player_iterator iter)override{
                ctx.add_to_pot( iter->stack() );
                iter->state() = PlayerState_AllIn;
                iter->stack() = 0.0; 
                ++ctx.allin_count();
                --ctx.active_count();
        };
        void post_bb(hand_context& ctx, hand_context::player_iterator iter)override{
                ctx.add_to_pot( ctx.bb() );
                iter->stack() -= ctx.bb();
        };
        void post_bb_allin(hand_context& ctx, hand_context::player_iterator iter)override{
                ctx.add_to_pot( iter->stack() );
                iter->state() = PlayerState_AllIn;
                iter->stack() = 0.0; 
                ++ctx.allin_count();
                --ctx.active_count();
        };
};
struct player_print_controller : player_controller{
        virtual ~player_print_controller()=default;
        void push         (hand_context& ctx, hand_context::player_iterator iter)override{
                std::cout << iter->name() << " is all in for " << iter->stack() << "\n";
        }
        void fold         (hand_context& ctx, hand_context::player_iterator iter)override{
                std::cout << iter->name() << " fold\n";
        }
        void post_sb      (hand_context& ctx, hand_context::player_iterator iter)override{
                std::cout << iter->name() << " posts sb of " << ctx.sb() << "\n";
        }
        void post_sb_allin(hand_context& ctx, hand_context::player_iterator iter)override{
                std::cout << iter->name() << " posts sb of " << ctx.sb() << "\n";
        }
        void post_bb      (hand_context& ctx, hand_context::player_iterator iter)override{
                std::cout << iter->name() << " posts bb of " << ctx.bb() << "\n";
        }
        void post_bb_allin(hand_context& ctx, hand_context::player_iterator iter)override{
                std::cout << iter->name() << " posts bb of " << ctx.bb() << "\n";
        }
};

#if 0
// apply action
switch(act.action()){
case PlayerAction_PostSB:
        post_pot_( decl_.get_sb() );
        iter_->stack() -= ctx_->sb();
        break;
case PlayerAction_PostSBAllin:
        post_pot_( iter_->stack() );
        iter_->state() = PlayerState_AllIn;
        iter_->stack() = 0.0; 
        ++ctx_->allin_count();
        --ctx_->active_count();
        break;
case PlayerAction_PostBB:
        post_pot_( decl_.get_bb() );
        iter_->stack() -= decl_.get_bb();
        break;
case PlayerAction_PostBBAllin:
        post_pot_( iter_->stack() );
        iter_->state() = PlayerState_AllIn;
        iter_->stack() = 0.0; 
        ++ctx_->allin_count();
        --ctx_->active_count();
        break;
case PlayerAction_Push:
        post_pot_( iter_->stack() );
        iter_->state() = PlayerState_AllIn;
        iter_->stack() = 0.0; 
        ++ctx_->allin_count();
        --ctx_->active_count();
        break;
case PlayerAction_Fold:
        iter_->state() = PlayerState_Fold;
        --ctx_->active_count();
        break;
#endif

namespace actions{

        struct post_sb{
                void execute(hand_context& ctx,
                             hand_context::player_iterator iter,
                             player_controller& ctrl)const
                {
                        if( iter->stack() <= ctx.sb() ){
                                ctrl.post_sb_allin(ctx, iter);
                        } else {
                                ctrl.post_sb(ctx, iter);
                        }
                }
        };
        struct post_bb{
                void execute(hand_context& ctx,
                             hand_context::player_iterator iter,
                             player_controller& ctrl)const
                {
                        if( iter->stack() <= ctx.bb() ){
                                ctrl.post_bb_allin(ctx, iter);
                        } else {
                                ctrl.post_bb(ctx, iter);
                        }
                }
        };
        struct push{
                void execute(hand_context& ctx,
                             hand_context::player_iterator iter,
                             player_controller& ctrl)const
                {
                        ctrl.push(ctx, iter);
                }
        };
        struct fold{
                void execute(hand_context& ctx,
                             hand_context::player_iterator iter,
                             player_controller& ctrl)const
                {
                        ctrl.fold(ctx, iter);
                }
        };



} // actions

using any_action = boost::variant<
        actions::post_sb,
        actions::post_bb,
        actions::push,
        actions::fold
>;
static auto post_sb_ = actions::post_sb();
static auto post_bb_ = actions::post_bb();
static auto push_    = actions::push();
static auto fold_    = actions::fold();



struct action_t{
        template<class Action>
        action_t(Action&& a):impl_{std::forward<Action>(a)}{}

        void execute(hand_context& ctx, hand_context::player_iterator iter,
                     player_controller& ctrl)const{
                boost::apply_visitor( [&](auto& _)->void{
                        _.execute(ctx, iter, ctrl);
                }, impl_);
        }
private:
        any_action impl_;
};


struct hand_ledger{
        explicit hand_ledger(hand_context const& init_ctx):
                init_ctx_{init_ctx}
        {}
        // replay the ledger from the start
        void replay(player_controller& ctrl)
        {
                auto ctx = init_ctx_;

                auto iter = ctx.begin();
                auto end = ctx.end();

                for(auto & a : v_){
                        a.execute(ctx, iter, ctrl);
                        ++iter;
                        if( iter == end ){
                                std::cerr << "broke early\n";
                                break;
                        }
                }
        }
        void post(action_t a){
                v_.push_back(std::move(a));
        }
private:
        hand_context init_ctx_;
        std::vector<action_t> v_;
};

struct hand_controller{
        hand_controller(hand_context& ctx,
                        hand_ledger& ledger)
                :ledger_{&ledger}
                ,ctx_{&ctx}
                ,iter_{ctx_->begin()}
                ,end_{ctx_->end()}
        {}
        // execute action
        void execute(action_t a){
                a.execute(*ctx_, iter_, ctrl_);
                ++iter_;
                ledger_->post(std::move(a));
        }
        size_t cursor(){ return iter_->idx(); }
        // end of hand
        bool eoh()const{ 
                return iter_ == end_;
        }
private:
        hand_ledger* ledger_;
        hand_context* ctx_;
        player_controller_default ctrl_;
        hand_context::player_iterator iter_;
        hand_context::player_iterator end_;
};

// {{{
struct player_strat{
        virtual ~player_strat()=default;
        virtual any_action act(hand_context const& ctx, hand_ledger const& ledger, hand_context::player_iterator iter)=0;
};
struct push_player_strat : player_strat{
        any_action act(hand_context const& ctx, hand_ledger const& ledger, hand_context::player_iterator iter)override{
                return actions::push{};
        }
};
struct fold_player_strat : player_strat{
        any_action act(hand_context const& ctx, hand_ledger const& ledger, hand_context::player_iterator iter)override{
                return actions::fold{};
        }
};
#if 0
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
        any_action act(hand_context const& ctx, hand_ledger const& ledger, holdem_id id)override{
                PlayerAction act = PlayerAction_Fold;
                auto class_ = holdem_hand_decl::get(id).class_();
                if(ctx.cursor() == ctx.sb_offset()){
                        // sb opening action
                        if( sb_strat_[class_] >= ctx.get_decl().get_stacks()[0]){
                                return actions::push{}
                        }
                } else{
                        // bb facing a push
                        if( bb_strat_[class_] >=ctx.get_decl().get_stacks()[0]){ 
                                return actions::push{}
                        }
                }
                return actions::fold{}
        }
private:
        holdem_class_strategy sb_strat_;
        holdem_class_strategy bb_strat_;
};
#endif

struct dealer{
        explicit dealer(size_t n, size_t initial_btn)
                :n_{n}
                ,btn_{initial_btn}
        {}
        size_t shuffle_and_deal_btn(){
                removed_ = 0;
                auto btn = btn_;
                ++btn_;
                btn_ = btn_ % n_;
                return btn;
        }
        holdem_id deal(){
                auto x = deal_card_();
                auto y = deal_card_();
                auto id = holdem_hand_decl::make_id(x, y);
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
        size_t removed_{0};
        size_t btn_{0};
};

void game_context_test(){
        game_decl decl(0.5, 1.0);
        decl.push_player(10);
        decl.push_player(10);
        decl.push_player(10);
        dealer d(3,0);
        auto btn = d.shuffle_and_deal_btn();
        hand_context ctx(decl, 0);
        ctx.display();
        hand_ledger ledger(ctx);;
        hand_controller ctrl(ctx, ledger);
        ctrl.execute(post_sb_);
        ctrl.execute(post_bb_);
        ctrl.execute(push_);
        ctrl.execute(push_);
        PRINT( ctrl.eoh() );
        ctrl.execute(fold_);
        PRINT( ctrl.eoh() );
        ctx.display();

        player_print_controller pp;
        ledger.replay(pp);

}

int main(){
        game_context_test();
}

#if 0

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
        std::vector<double> eval(hand_context const& ctx, std::vector<holdem_id> const& deal){
                std::vector<double> d(ctx.players_size());
                
                do{
                        if(ctx.allin_count()==0){
                                // case walk
                                //PRINT("walk");
                                d[ctx.sb_offset()] -= ctx.sb();
                                d[ctx.bb_offset()] += ctx.sb();
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
                                d[ctx.sb_offset()] -= ctx.sb();
                                d[p[0]]            += ctx.sb();
                                d[ctx.bb_offset()] -= ctx.bb();
                                d[p[0]]            += ctx.bb();
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
                                d[ctx.sb_offset()] -= ctx.sb();
                        if( ctx.get_player(ctx.bb_offset()).state() != PlayerState_AllIn )
                                d[ctx.bb_offset()] -= ctx.bb();
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
                hand_context ctx(decl_, btn);
                hand_ledger ledger;
                hand_controller ctrl(ctx, ledger);

                ctrl.post_blinds();

                holdem_hand_vector deal;
                deal.resize(ctx.n());
                for(size_t i=0;i!=deal.size(); ++i){
                        deal[i] = dealer_.deal();
                }

                for( ; ctx.players_left_to_act();){
                        auto id = ctrl.cursor();
                        auto a = p_[id]->act(ctx, ledger, deal[id]);
                        ctrl.execute(a);
                }
                //ctx.display();

                auto ret = ge_.eval(ctx, deal);

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

        #if 1
        auto pf_strat = std::make_shared<holdem_class_strat_player>(sb_strat, bb_strat);

        pv.push_back(pf_strat);
        pv.push_back(pf_strat);
        #endif
        #if 0
        pv.push_back(std::make_shared<fold_player_strat>());
        pv.push_back(std::make_shared<push_player_strat>());
        #endif

        simultation_context sim(decl, pv);


        std::vector<double> d(2);

        
        for(;;){
        //for(size_t j=0;j!=10;++j){
                for(size_t i=0;i!=10;++i){
                        auto r =sim.simulate();
                        for(size_t j=0;j!=2;++j){
                                d[j] += r[j];
                        }
                        //PRINT(detail::to_string(r));
                }
                PRINT(detail::to_string(d));
        }


        //auto result = sim.result();
}
/// }}}

#endif
