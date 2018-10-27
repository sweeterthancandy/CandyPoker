#include <thread>
#include <numeric>
#include <atomic>
#include <bitset>
#include <fstream>
#include <unordered_map>

#include <boost/format.hpp>
#include <boost/assert.hpp>

#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/base/cards.h"
#include "ps/base/cards.h"
#include "ps/base/frontend.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/tree.h"

#include "ps/detail/tree_printer.h"

#include "ps/eval/class_cache.h"
#include "ps/eval/pass_mask_eval.h"
#include "ps/eval/instruction.h"

#include "ps/support/config.h"
#include "ps/support/index_sequence.h"

#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>

#include "ps/support/command.h"

#include <boost/iterator/indirect_iterator.hpp>
#include "ps/eval/holdem_class_vector_cache.h"


/*
 * 2 player
        
        player ev
        =========
 
        ---+---------------
        f_ | 0
        pf | sb+bb
        pp | 2*S*Ev - (S-sb)

        ---+---------------
        pf | 0
        pp | 2*S*Ev - (S-bb)
        

        total value
        ===========


        ---+---------------
        f_ | -sb
        pf | bb
        pp | 2*S*Ev - S
        
        ---+---------------
        pf | -sb
        pp | 2*S*Ev - S





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



                f(2) -> g({pp,pf,fp,ff}) -> {pp,pf,fp,ff} \ { perm : all actions fold before last player }
                                                             \------- always size 2 (n, as f*f, f*p -----/

                => |f(2)| = |{pp,pf,fp,ff}| - |{fp,ff}| 
                => |f(3)| = |{ppp,ppf,pfp,pff,fpp,fpf,ffp,fff}| - |{ffp,fff}| = 2 ^ 3 - 2 = 6

                    f(3) = {ppp,ppf,pfp,pff,fpp,fpf,ff}
                        
                    q(0) = {ppp,ppf,pfp,pff},{fpp,fpf,ff}
                            ^   ^   ^   ^     ^   ^   ^

                    q(1) = {ppp,ppf},{pfp,pff},{fpp,fpf},{ff}
                            _^  _^    _^  _^    _^  _^    _^
                    
                    q(2) = {ppp},{ppf},{pfp},{pff},{fpp},{fpf },{ff}
                            __^   __^   __^   __^   __^   __^



 */

namespace ps {
namespace gt{


        struct eval_tree_node;

        struct gt_context{
                gt_context(size_t num_players, double eff, double sb, double bb)
                        :num_players_{num_players},
                        eff_(eff),
                        sb_(sb),
                        bb_(bb)
                {}
                size_t num_players()const{ return num_players_; }
                double eff()const{ return eff_; }
                double sb()const{ return sb_; }
                double bb()const{ return bb_; }

                gt_context& use_game_tree(std::shared_ptr<eval_tree_node> gt){
                        gt_ = gt;
                        return *this;
                }
                gt_context& use_cache(class_cache const& cc){
                        cc_ = &cc;
                        return *this;
                }
                
                std::shared_ptr<eval_tree_node> root()const{
                        return gt_;
                }
                class_cache const* cc()const{
                        return cc_;
                }
                
                friend std::ostream& operator<<(std::ostream& ostr, gt_context const& self){
                        ostr << "eff_ = " << self.eff_;
                        ostr << ", sb_ = " << self.sb_;
                        ostr << ", bb_ = " << self.bb_;
                        return ostr;
                }
        private:
                size_t num_players_;
                double eff_;
                double sb_;
                double bb_;

                std::shared_ptr<eval_tree_node> gt_;
                class_cache const* cc_;
        };
        
        struct eval_tree_node;

        /*
                Need a way to abstract the strategy represention.
                
                For a hu game, we have a representation of a 2-vector,
                each of size 169, which a value \in [0,1] which represents
                the probabilty of pushing.

                For a three player game, we have a 6-vector of 169-vectors,
                which each value \in [0,1] which reprents of the probaily
                of pushing, given the previous action
         */
        struct binary_strategy_description{
                using strategy_impl_t = std::vector<Eigen::VectorXd>;

                // 
                virtual double sb()const=0;
                virtual double bb()const=0;
                virtual double eff()const=0;
                virtual size_t num_players()const=0;

                virtual strategy_impl_t make_inital_state()const=0;
                /*
                 *  We are constraied to only events which can be represented by a key,
                 *  for push/fold solving this is appropraite.
                 */

                struct event_decl{
                        virtual ~event_decl()=default;
                        virtual std::string key()const=0;
                        virtual void value_of_event(Eigen::VectorXd& out, holdem_class_vector const& cv, double p = 1.0)const=0;
                        virtual void expected_value_of_event(binary_strategy_description const* desc, Eigen::VectorXd& out, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                                auto p = desc->probability_of_event(key(), cv, impl);
                                value_of_event(out, cv, p);
                        }
                        double probability_of_event(binary_strategy_description* desc, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                                return desc->probability_of_event(key(), cv, impl);
                        }
                        virtual std::string to_string()const=0;
                };
                using event_vector = std::vector<std::shared_ptr<event_decl> >;
                using event_iterator = boost::indirect_iterator<event_vector::const_iterator>;
                event_iterator begin_event()const{ return events_.begin(); }
                event_iterator end_event()const{ return events_.end(); }

                // the ev GIVEN a certain deal
                //    result = \sum P(q) * E[q]
                Eigen::VectorXd expected_value_of_vector(holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                        Eigen::VectorXd vec(num_players()+1);
                        vec.fill(0);
                        for(auto iter(begin_event()),end(end_event());iter!=end;++iter){
                                iter->expected_value_of_event(this, vec, cv, impl);
                        }
                        return vec;
                }
                virtual Eigen::VectorXd expected_value_by_class_id(size_t player_idx, strategy_impl_t const& impl)const=0;

                virtual double probability_of_event(std::string const& key, holdem_class_vector const& cv, strategy_impl_t const& impl)const=0;

                void check_probability_of_event(holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                        double sigma = 0.0;
                        for(auto iter(begin_event()),end(end_event());iter!=end;++iter){
                                sigma += probability_of_event(iter->key(), cv, impl);
                        }
                        double epsilon = 1e-4;
                        if( std::fabs(sigma - 1.0) > epsilon ){
                                std::stringstream sstr;
                                sstr << std::fixed;
                                sstr << "bad events sigma=" << sigma;
                                throw std::domain_error(sstr.str());
                        }
                }

                struct strategy_decl{
                        strategy_decl(size_t vec_idx, size_t player_idx, std::string const& desc)
                                :vec_idx_(vec_idx),
                                player_idx_(player_idx),
                                desc_(desc)
                        {}
                        size_t vector_index()const{ return vec_idx_; }
                        size_t player_index()const{ return player_idx_; }
                        std::string const& description()const{ return desc_; }
                        strategy_impl_t make_all_fold(strategy_impl_t const& impl)const{
                                auto result = impl;
                                result[vec_idx_].fill(0);
                                return result;
                        }
                        strategy_impl_t make_all_push(strategy_impl_t const& impl)const{
                                auto result = impl;
                                result[vec_idx_].fill(1);
                                return result;
                        }

                private:
                        size_t vec_idx_;
                        size_t player_idx_;
                        std::string desc_;
                };
                using strategy_vector = std::vector<strategy_decl>;
                using strategy_iterator = strategy_vector::const_iterator;
                strategy_iterator begin_strategy()const{ return strats_.begin(); }
                strategy_iterator end_strategy()const{ return strats_.end(); }
        protected:
                event_vector events_;
                strategy_vector strats_;

        };
        struct static_event : binary_strategy_description::event_decl{
                explicit static_event(std::string const& key, Eigen::VectorXd vec):
                        key_{key}, vec_{vec}
                {}
                virtual std::string key()const override{ return key_; }
                virtual void value_of_event(Eigen::VectorXd& out, holdem_class_vector const& cv, double p)const override{
                        for(size_t idx=0;idx!=vec_.size();++idx){
                                out[idx] += vec_[idx] * p;
                        }
                }
                virtual std::string to_string()const override{
                        std::stringstream sstr;
                        sstr << "Static{key=" << key_ << ", vec_=" << vector_to_string(vec_) << "}";
                        return sstr.str();
                }
        private:
                std::string key_;
                Eigen::VectorXd vec_;
        };
        struct eval_event : binary_strategy_description::event_decl{
                enum{ Debug = 0 };
                explicit
                eval_event(class_cache const* cc,
                           std::string const& key,
                           std::vector<size_t> perm,
                           Eigen::VectorXd const& dead_money,
                           Eigen::VectorXd const& active)
                        :cc_{cc}, key_(key), perm_{perm}, dead_money_{dead_money}, active_{active}
                        ,pot_amt_{active_.sum() + dead_money_.sum()}
                {

                        delta_proto_.resize(dead_money_.size()+1);
                        delta_proto_.fill(0);
                        for(size_t idx=0;idx!=active_.size();++idx){
                                delta_proto_[idx] -= active_[idx];
                                delta_proto_[idx] -= dead_money_[idx];
                        }
                        
                        if( Debug ){
                                std::cout << "perm => " << detail::to_string(perm) << "\n"; // __CandyPrint__(cxx-print-scalar,perm)
                                std::cout << "dead_money_ => " << vector_to_string(dead_money_) << "\n"; // __CandyPrint__(cxx-print-scalar,dead_money_)
                                std::cout << "active_ => " << vector_to_string(active_) << "\n"; // __CandyPrint__(cxx-print-scalar,active_)
                                std::cout << "delta_proto_ => " << vector_to_string(delta_proto_) << "\n"; // __CandyPrint__(cxx-print-scalar,delta_proto_)
                                std::cout << "pot_amt_ => " << pot_amt_ << "\n"; // __CandyPrint__(cxx-print-scalar,pot_amt_)
                        }

                }
                virtual std::string key()const override{ return key_; }
                virtual void value_of_event(Eigen::VectorXd& out, holdem_class_vector const& cv, double p)const override{

                        // short circuit for optimization purposes
                        if( std::fabs(p) < 0.001 )
                                return;

                        holdem_class_vector tmp;
                        for(auto _ : perm_ ){
                                tmp.push_back(cv[_]);
                        }
                        
                        auto ev = cc_->LookupVector(tmp);
                        

                        Eigen::VectorXd delta = delta_proto_;

                        size_t ev_idx = 0;
                        for( auto _ :perm_ ){
                                delta[_] += pot_amt_ * ev[ev_idx];
                                ++ev_idx;
                        }

                        //std::cout << "key=" << key_ << ",p" << p << ", cv=" << cv << ", delta=" << vector_to_string(delta) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(delta))
                        delta *= p;

                        out += delta;
                }
                virtual std::string to_string()const override{
                        std::stringstream sstr;
                        sstr << std::fixed;
                        sstr << "Eval  {key=" << key_ << ", perm=" << detail::to_string(perm_) 
                                << ", dead=" << vector_to_string(dead_money_)
                                << ", active=" << vector_to_string(active_)
                                << ", proto=" << vector_to_string(delta_proto_)
                                << ", pot=" << pot_amt_ 
                                << "}";
                        return sstr.str();
                }
        private:
                class_cache const* cc_;
                std::string key_;
                std::vector<size_t> perm_;
                Eigen::VectorXd dead_money_;
                Eigen::VectorXd active_;
                Eigen::VectorXd delta_proto_;
                double pot_amt_;
        };


        struct heads_up_description : binary_strategy_description{
                heads_up_description(double sb, double bb, double eff)
                        : sb_{sb}, bb_{bb}, eff_{eff}
                {

	
                        std::string cache_name{".cc.bin"};
                        cc_.load(cache_name);


                        Eigen::VectorXd v_f_{2};
                        v_f_(0) = -sb_;
                        v_f_(1) =  sb_;
                        auto n_f_ = std::make_shared<static_event>("f", v_f_);
                        events_.push_back(n_f_);

                        Eigen::VectorXd v_pf{2};
                        v_pf(0) =  bb_;
                        v_pf(1) = -bb_;
                        auto n_pf = std::make_shared<static_event>("pf", v_pf);
                        events_.push_back(n_pf);

                        Eigen::VectorXd dead_money = Eigen::VectorXd::Zero(2);
                        Eigen::VectorXd active{2};
                        active[0] = eff_;
                        active[1] = eff_;
                        auto n_pp = std::make_shared<eval_event>(&cc_, "pp", std::vector<size_t>{0,1}, dead_money, active);
                        events_.push_back(n_pp);

                        strats_.emplace_back(0,0, "SB Pushing");
                        strats_.emplace_back(1,1, "BB Calling, given a SB push");
                }
                virtual strategy_impl_t make_inital_state()const override{
                        Eigen::VectorXd proto(169);
                        proto.fill(0.0);
                        strategy_impl_t vec;
                        vec.emplace_back(proto);
                        vec.emplace_back(proto);
                        return vec;
                }
                virtual double sb()const{ return sb_; }
                virtual double bb()const{ return bb_; }
                virtual double eff()const{ return eff_; }
                virtual size_t num_players()const{ return 2; }
                
                virtual double probability_of_event(std::string const& key, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                        auto a = impl[0][cv[0]];
                        auto b = impl[1][cv[1]];
                        //if( key == "p" ){ return a; }
                        if( key == "f" ){ return ( 1.0 - a); }
                        if( key == "pp"){ return a * b; }
                        if( key == "pf"){ return a * (1.0 - b ); }
                        std::stringstream sstr;
                        sstr << "unknown key " << key;
                        throw std::domain_error(sstr.str());
                }
                virtual Eigen::VectorXd expected_value_by_class_id(size_t player_idx, strategy_impl_t const& impl)const override{
                        Eigen::VectorXd result(169);
                        result.fill(0);
                        for(holdem_class_perm_iterator iter(2),end;iter!=end;++iter){
                                auto const& cv = *iter;
                                check_probability_of_event(cv, impl);
                                auto p = cv.prob();
                                auto ev = expected_value_of_vector(cv, impl);
                                result(cv[player_idx]) += p * ev[player_idx];
                        }
                        return result;
                }
        private:
                double sb_;
                double bb_;
                double eff_;

                class_cache cc_;
        };
        
        
        struct three_player_description : binary_strategy_description{
                three_player_description(double sb, double bb, double eff)
                        : sb_{sb}, bb_{bb}, eff_{eff}
                {

	
                        std::string cache_name{".cc.bin"};
                        cc_.load(cache_name);


                        size_t num_players = 3;
                                

                        Eigen::VectorXd stacks{num_players};
                        for(size_t idx=0;idx!=num_players;++idx){
                                stacks[idx] = eff_;
                        }


                        Eigen::VectorXd v_blinds{num_players};
                        v_blinds.fill(0.0);
                        v_blinds[1] = sb_;
                        v_blinds[2] = bb_;

                        auto make_static = [&](std::string const& key, size_t target){
                                Eigen::VectorXd sv = -v_blinds;
                                sv[target] += v_blinds.sum();
                                auto ptr = std::make_shared<static_event>(key, sv);
                                return ptr;
                        };

                        for(unsigned long long mask = ( 1 << num_players ); mask != 0;){
                                --mask;
                                std::bitset<32> bs = {mask};


                                std::string key;
                                std::vector<size_t> perm;
                                Eigen::VectorXd dead_money = Eigen::VectorXd::Zero(3);
                                Eigen::VectorXd active     = Eigen::VectorXd::Zero(3);

                                for(size_t idx=0;idx!= num_players;++idx){
                                        if( bs.test(idx) ){
                                                active[idx] = stacks[idx];
                                                perm.push_back(idx);
                                                key += "p";
                                        } else{
                                                dead_money[idx] = v_blinds[idx];
                                                key += "f";
                                        }
                                }

                                if( bs.count() == 0 )
                                        continue;
                                if( bs.count() == 1 && bs.test(num_players-1) ){
                                        // walk
                                        std::string degenerate_key(num_players-1, 'f');
                                        auto walk = make_static(degenerate_key, num_players-1);
                                        events_.push_back(walk);
                                } else if( bs.count() == 1 ){
                                        // steal 
                                        auto steal = make_static(key, perm[0]);
                                        events_.push_back(steal);
                                } else { 
                                        // push call

                                        auto allin = std::make_shared<eval_event>(&cc_, key, perm, dead_money, active);
                                        events_.push_back(allin);
                                }
                                
                        }

                        strats_.emplace_back(0,0, "BTN Pushing");
                        strats_.emplace_back(1,1, "SB Calling, given BTN Push");
                        strats_.emplace_back(2,1, "SB Pushing, given BTN Fold"); 
                        strats_.emplace_back(3,2, "BB Calling, given BTN Push, SB Call");
                        strats_.emplace_back(4,2, "BB Calling, given BTN Push, SB Fold");
                        strats_.emplace_back(5,2, "BB Calling, given BTN Fold, SB Push");
                }
                virtual strategy_impl_t make_inital_state()const override{
                        Eigen::VectorXd proto(169);
                        proto.fill(0.0);
                        strategy_impl_t vec;
                        vec.emplace_back(proto);
                        vec.emplace_back(proto);
                        vec.emplace_back(proto);
                        vec.emplace_back(proto);
                        vec.emplace_back(proto);
                        vec.emplace_back(proto);
                        return vec;
                }
                virtual double sb()const{ return sb_; }
                virtual double bb()const{ return bb_; }
                virtual double eff()const{ return eff_; }
                virtual size_t num_players()const{ return 3; }
                
                virtual double probability_of_event(std::string const& key, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                        enum{ Debug = 0 };
                        static std::unordered_map<std::string, size_t> reg_alloc = {
                                             //  Player
                                { ""  , 0 }, //   0
                                { "p" , 1 }, //   1
                                { "f" , 2 }, //   1
                                { "pp", 3 }, //   2
                                { "pf", 4 }, //   2
                                { "fp", 5 }  //   2
                        };
                        double result = 1.0;
                        std::string sub;
                        std::stringstream dbg;
                        for(size_t idx=0;idx!=key.size();++idx){
                                if( reg_alloc.count(sub) == 0 ){
                                        throw std::domain_error("bad");
                                }
                                auto reg = reg_alloc[sub];
                                switch(key[idx]){
                                case 'p':
                                case 'P':
                                {
                                        dbg << "P<" << reg << "," << ( impl[reg][cv[idx]] ) << ">";
                                        result *= impl[reg][cv[idx]];
                                        sub += 'p';
                                        break;
                                }
                                case 'f':
                                case 'F':
                                {
                                        dbg << "F<" << reg << "," << ( 1 - impl[reg][cv[idx]] )<<">";
                                        result *= ( 1 - impl[reg][cv[idx]] );
                                        sub += 'f';
                                        break;
                                }}
                        }
                        if( Debug ) std::cout << dbg.str() << "\n";
                        return result;
                }
                virtual Eigen::VectorXd expected_value_by_class_id(size_t player_idx, strategy_impl_t const& impl)const override{
                        Eigen::VectorXd result(169);
                        result.fill(0);
                        for(auto const& _ : *Memory_ThreePlayerClassVector){
                                auto const& cv = _.cv;
                                auto ev = expected_value_of_vector(cv, impl);
                                result(cv[player_idx]) += _.prob * ev[player_idx];
                        }
                        return result;
                }
        private:
                double sb_;
                double bb_;
                double eff_;

                class_cache cc_;
        };

        double eval_prob_from_key(std::string const& key, Eigen::VectorXd const& vec){
                enum{ Debug = 0 };
                static std::unordered_map<std::string, size_t> reg_alloc = {
                                     //  Player
                        { ""  , 0 }, //   0
                        { "p" , 1 }, //   1
                        { "f" , 2 }, //   1
                        { "pp", 3 }, //   2
                        { "pf", 4 }, //   2
                        { "fp", 5 }  //   2
                };
                double result = 1.0;
                std::string sub;
                std::stringstream dbg;
                for(size_t idx=0;idx!=key.size();++idx){
                        if( reg_alloc.count(sub) == 0 ){
                                throw std::domain_error("bad");
                        }
                        auto reg = reg_alloc[sub];
                        switch(key[idx]){
                        case 'p':
                        case 'P':
                        {
                                dbg << "P<" << reg << "," << ( vec[reg] ) << ">";
                                result *= vec[reg];
                                sub += 'p';
                                break;
                        }
                        case 'f':
                        case 'F':
                        {
                                dbg << "F<" << reg << "," << ( 1 - vec[reg] )<<">";
                                result *= ( 1 - vec[reg] );
                                sub += 'f';
                                break;
                        }}
                }
                if( Debug ) std::cout << dbg.str() << "\n";
                return result;
        }
        void __eval_prob_from_key_test(){
                Eigen::VectorXd v(6);
                double a =  0.2; // <nothing>
                double b =  0.3; // p
                double c =  0.5; // f
                double d =  0.7; // pp
                double e = 0.11; // pf
                double f = 0.13; // fp
                
                double A = ( 1.0 - a);
                double B = ( 1.0 - b);
                double C = ( 1.0 - c);
                double D = ( 1.0 - d);
                double E = ( 1.0 - e);
                double F = ( 1.0 - f);

                v[0] = a;
                v[1] = b;
                v[2] = c;
                v[3] = d;
                v[4] = e;
                v[5] = f;

                auto check = [](std::string const& expr_s, auto expr,
                                std::string const& exp_s, auto exp){
                        double epsilon = 1e-3;
                        if( ! (std::fabs(expr - exp) < epsilon ) ){
                                std::stringstream sstr;
                                sstr << std::fixed;
                                sstr << expr_s << "=" << expr << ", ";
                                sstr << exp_s << "=" << exp;
                                throw std::domain_error(sstr.str());
                        }
                };
                #define check(expr,exp) check(#expr, expr, #exp, exp)
                check( eval_prob_from_key("p"  , v), a    );
                check( eval_prob_from_key("f"  , v), A    );
                check( eval_prob_from_key("pp" , v), a*b  );
                check( eval_prob_from_key("pf" , v), a*B  );
                check( eval_prob_from_key("fp" , v), A*c  );
                check( eval_prob_from_key("ff" , v), A*C  );
                
                check( eval_prob_from_key("ppp" , v), a*b*d  );
                check( eval_prob_from_key("pfp" , v), a*B*e  );
                check( eval_prob_from_key("fpp" , v), A*c*f  );
                //check( eval_prob_from_key("ffp" , v), A*C  );
                check( eval_prob_from_key("ppf" , v), a*b*D  );
                check( eval_prob_from_key("pff" , v), a*B*E  );
                check( eval_prob_from_key("fpf" , v), A*c*F  );
                //check( eval_prob_from_key("fff" , v), A*C  );

                
                #undef check

        }
        static int __eval_prob_from_key_test_mem = ( __eval_prob_from_key_test(), 0 );



        struct eval_tree_node{
                /*
                        \param[out]  out   A vector of size ctx.num_players(), which will be 
                                           used to hold the probability weight result
  
                        \param[in]   ctx   A gt_context for sb,bb,eff etc

                        \param[in]   vec   A holdem class vector, representing the combination
                                           of hand deals between the players. For any N-player
                                           game, any hand deal can be represented of some 
                                           N-tuple of holdem class ids.

                        \param[in]   s     A probability realization of the strategy vector.
                                           This is equivalent to
                                                P(c=c0)P(c=c1|x)P(c=c2|xx)...,
                                           ie each 
                 */
                virtual void evaluate(Eigen::VectorXd& out,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)=0;
                virtual void display(std::ostream& ostr = std::cout)const=0;
        };
        struct eval_tree_node_static : eval_tree_node{
                explicit eval_tree_node_static(std::string const& key, Eigen::VectorXd vec):
                        key_{key}, vec_{vec}
                {}

                virtual void evaluate(Eigen::VectorXd& out,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)override
                {
                        auto p = eval_prob_from_key(key_, s);
                        //std::cout << "--" << key_ << " => " << p << "\n";
                        for(size_t idx=0;idx!=vec_.size();++idx){
                                out[idx] += vec_[idx] * p;
                        }
                        //out += vec_ * p;
                        out[vec.size()] += p;
                }
                virtual void display(std::ostream& ostr = std::cout)const override{
                        ostr << "Static{" << key_
                                          << ", " << vector_to_string(vec_) << "\n";
                }
        private:
                std::string key_;
                Eigen::VectorXd vec_;
        };

        #if 0
        struct eval_tree_node_eval : eval_tree_node{
                template<class... Args>
                explicit
                eval_tree_node_eval(Args&&...)
                {
                        v_mask_.resize(2);
                        v_mask_.fill(1);
                }
                virtual void evaluate(Eigen::VectorXd& out,
                                      double p,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)override
                {
                        auto q = factor(s);
                        p *= q;
                        if( std::fabs(p) < 0.001 )
                                return;

                        auto ev = ctx.cc()->LookupVector(vec);

                        auto equity_vec = ( v_mask_.size() * ev - v_mask_ ) * ctx.eff() * p;

                        out += equity_vec;
                        out[vec.size()] += p;
                }
                virtual void display(std::ostream& ostr = std::cout)const override{
                        ostr << "Eval{" << vector_to_string(v_mask_) << "\n";
                }
        private:
                Eigen::VectorXd v_mask_;
        };
        #endif

        struct eval_tree_node_eval : eval_tree_node{
                enum{ Debug = 0 };
                explicit
                eval_tree_node_eval(std::string const& key,
                                    std::vector<size_t> perm,
                                    Eigen::VectorXd const& dead_money,
                                    Eigen::VectorXd const& active)
                        :key_(key), perm_{perm}, dead_money_{dead_money}, active_{active}
                        ,pot_amt_{active_.sum() + dead_money_.sum()}
                {

                        delta_proto_.resize(dead_money_.size()+1);
                        delta_proto_.fill(0);
                        for(size_t idx=0;idx!=active_.size();++idx){
                                delta_proto_[idx] -= active_[idx];
                                delta_proto_[idx] -= dead_money_[idx];
                        }
                        
                        if( Debug ){
                                std::cout << "perm => " << detail::to_string(perm) << "\n"; // __CandyPrint__(cxx-print-scalar,perm)
                                std::cout << "dead_money_ => " << vector_to_string(dead_money_) << "\n"; // __CandyPrint__(cxx-print-scalar,dead_money_)
                                std::cout << "active_ => " << vector_to_string(active_) << "\n"; // __CandyPrint__(cxx-print-scalar,active_)
                                std::cout << "delta_proto_ => " << vector_to_string(delta_proto_) << "\n"; // __CandyPrint__(cxx-print-scalar,delta_proto_)
                                std::cout << "pot_amt_ => " << pot_amt_ << "\n"; // __CandyPrint__(cxx-print-scalar,pot_amt_)
                        }

                }
                virtual void evaluate(Eigen::VectorXd& out,
                                      gt_context const& ctx,
                                      holdem_class_vector const& cv,
                                      Eigen::VectorXd const& s)override
                {
                        auto p = eval_prob_from_key(key_, s);

                        //std::cout << "--" << key_ << " => " << p << "\n";

                        // short circuit for optimization purposes
                        if( std::fabs(p) < 0.001 )
                                return;

                        holdem_class_vector tmp;
                        for(auto _ : perm_ ){
                                tmp.push_back(cv[_]);
                        }
                        
                        auto ev = ctx.cc()->LookupVector(tmp);
                        

                        Eigen::VectorXd delta = delta_proto_;

                        size_t ev_idx = 0;
                        for( auto _ :perm_ ){
                                delta[_] += pot_amt_ * ev[ev_idx];
                                ++ev_idx;
                        }

                        //std::cout << "key=" << key_ << ",p" << p << ", cv=" << cv << ", delta=" << vector_to_string(delta) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(delta))
                        delta *= p;


                        out += delta;
                        // for checking
                        out[ctx.num_players()] += p;
                }
                virtual void display(std::ostream& ostr = std::cout)const override{
                        ostr << "Eval{" << key_ 
                                        << ", " << detail::to_string(perm_)
                                        << ", " << vector_to_string(dead_money_)
                                        << ", " << vector_to_string(active_) 
                             << "}\n";
                }
        private:
                std::string key_;
                std::vector<size_t> perm_;
                Eigen::VectorXd dead_money_;
                Eigen::VectorXd active_;
                Eigen::VectorXd delta_proto_;
                double pot_amt_;
        };

        struct eval_tree_non_terminal
                : public eval_tree_node
                , public std::vector<std::shared_ptr<eval_tree_node> >
        {
                virtual void evaluate(Eigen::VectorXd& out,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)override
                {
                        //std::cout << "Begin{}\n";
                        for(auto& ptr : *this){
                                ptr->evaluate(out, ctx, vec, s);
                        }
                        //std::cout << "End{}\n";
                }
                virtual void display(std::ostream& ostr = std::cout)const override{
                        ostr << "Begin{}\n";
                        for(auto const& ptr : *this){
                                ptr->display(ostr);
                        }
                        ostr << "End{}\n";
                }
        };
        
        struct hu_eval_tree_flat : eval_tree_non_terminal{
                explicit
                hu_eval_tree_flat( gt_context const& ctx){
                        Eigen::VectorXd v_f_{2};
                        v_f_(0) = -ctx.sb();
                        v_f_(1) =  ctx.sb();
                        auto n_f_ = std::make_shared<eval_tree_node_static>("f", v_f_);
                        push_back(n_f_);

                        Eigen::VectorXd v_pf{2};
                        v_pf(0) =  ctx.bb();
                        v_pf(1) = -ctx.bb();
                        auto n_pf = std::make_shared<eval_tree_node_static>("pf", v_pf);
                        push_back(n_pf);

                        Eigen::VectorXd dead_money = Eigen::VectorXd::Zero(2);
                        Eigen::VectorXd active{2};
                        active[0] = ctx.eff();
                        active[1] = ctx.eff();
                        auto n_pp = std::make_shared<eval_tree_node_eval>("pp", std::vector<size_t>{0,1}, dead_money, active);
                        //auto n_pp = std::make_shared<eval_tree_node_eval>();
                        push_back(n_pp);
                }
        };

        #if 0
        struct hu_eval_tree : eval_tree_non_terminal{
                explicit
                hu_eval_tree( gt_context const& ctx){


                        Eigen::VectorXd v_f_{2};
                        v_f_(0) = -ctx.sb();
                        v_f_(1) =  ctx.sb();
                        auto n_f_ = std::make_shared<eval_tree_node_static>(v_f_);
                        n_f_->not_times(0);
                        push_back(n_f_);

                        auto n_p_ = std::make_shared<eval_tree_non_terminal>();
                        n_p_->times(0);

                        Eigen::VectorXd v_pf{2};
                        v_pf(0) =  ctx.bb();
                        v_pf(1) = -ctx.bb();
                        auto n_pf = std::make_shared<eval_tree_node_static>(v_pf);
                        n_pf->not_times(1);
                        n_p_->push_back(n_pf);

                        Eigen::VectorXd dead_money = Eigen::VectorXd::Zero(2);
                        Eigen::VectorXd active{2};
                        active[0] = ctx.eff();
                        active[1] = ctx.eff();
                        auto n_pp = std::make_shared<eval_tree_node_eval>(std::vector<size_t>{0,1}, dead_money, active);
                        //auto n_pp = std::make_shared<eval_tree_node_eval>();
                        n_pp->times(1);
                        n_p_->push_back(n_pp);

                        push_back(n_p_);

                }
        };
        #endif

        struct three_way_eval_tree : eval_tree_non_terminal{
                explicit
                three_way_eval_tree( gt_context const& ctx){

                        // p p p 

                        size_t num_players = 3;
                                

                        Eigen::VectorXd stacks{num_players};
                        for(size_t idx=0;idx!=num_players;++idx){
                                stacks[idx] = ctx.eff();
                        }


                        Eigen::VectorXd v_blinds{num_players};
                        v_blinds.fill(0.0);
                        v_blinds[1] = ctx.sb();
                        v_blinds[2] = ctx.bb();

                        auto make_static = [&](std::string const& key, size_t target){
                                Eigen::VectorXd sv = -v_blinds;
                                sv[target] += v_blinds.sum();
                                auto ptr = std::make_shared<eval_tree_node_static>(key, sv);
                                return ptr;
                        };

                        for(unsigned long long mask = ( 1 << num_players ); mask != 0;){
                                --mask;
                                std::bitset<32> bs = {mask};


                                std::string key;
                                std::vector<size_t> perm;
                                Eigen::VectorXd dead_money = Eigen::VectorXd::Zero(3);
                                Eigen::VectorXd active     = Eigen::VectorXd::Zero(3);

                                for(size_t idx=0;idx!= num_players;++idx){
                                        if( bs.test(idx) ){
                                                active[idx] = stacks[idx];
                                                perm.push_back(idx);
                                                key += "p";
                                        } else{
                                                dead_money[idx] = v_blinds[idx];
                                                key += "f";
                                        }
                                }

                                if( bs.count() == 0 )
                                        continue;
                                if( bs.count() == 1 && bs.test(num_players-1) ){
                                        // walk
                                        std::string degenerate_key(num_players-1, 'f');
                                        auto walk = make_static(degenerate_key, num_players-1);
                                        push_back(walk);
                                } else if( bs.count() == 1 ){
                                        // steal 
                                        auto steal = make_static(key, perm[0]);
                                        push_back(steal);
                                } else { 
                                        // push call

                                        auto allin = std::make_shared<eval_tree_node_eval>(key, perm, dead_money, active);
                                        push_back(allin);
                                }
                                
                        }
                }
        };
        

        // returns a vector each players hand value
        Eigen::VectorXd combination_value(gt_context const& ctx,
                                          holdem_class_vector const& vec,
                                          Eigen::VectorXd const& s){
                Eigen::VectorXd result{vec.size()+1};
                result.fill(.0);
                ctx.root()->evaluate(result, ctx, vec,s);
                //std::cout << "result[vec.size()] => " << result[vec.size()] << "\n"; // __CandyPrint__(cxx-print-scalar,result[vec.size()])

                return result;
        }

        /*
                \param[in]  idx  The index of the strategy vector, for example
                                 for hu index 0 is for sb to push, whilst index
                                 1 is for bb to call a push (given the action p),
                                 ie 
                                        Index |Player|  key  | Given |   P
                                        ------+------+-------+-------+------
                                          0   |   0  |   p   |       | P(p)
                                          1   |   1  |   pp  |   p   | P(p|p)

                                For three player this canonical mapping doesn't
                                apply, we have
                                        
                                        Index |Player|  Key  | Given |   P
                                        ------+------+-------+-------+------
                                          0   |   0  |   p   |       | P(p)
                                          1   |   1  |   pp  |   p   | P(p|p)
                                          2   |   1  |   fp  |   f   | P(p|f)
                                          3   |   2  |   ppp |   pp  | P(p|pp)
                                          4   |   2  |   pfp |   pf  | P(p|pf)
                                          5   |   2  |   fpp |   fp  | P(p|fp)
        */
        Eigen::VectorXd unilateral_detail(gt_context const& ctx,
                                          size_t idx,
                                          std::vector<Eigen::VectorXd> const& S)
        {
                Eigen::VectorXd result(169);
                result.fill(.0);
                        
                Eigen::VectorXd s(S.size());

                auto player_idx = [](auto id){
                        switch(id){
                        case 0:
                                return 0;
                        case 1:
                        case 2:
                                return 1;
                        case 3:
                        case 4:
                        case 5:
                                return 2;
                        }
                        PS_UNREACHABLE();
                };

                if( ctx.num_players() == 3 ){
                        for(auto const& _ : *Memory_ThreePlayerClassVector){
                                auto const& cv = _.cv;
                                // create a view of the vector, nothing fancy
                                //
                                // The strategy vector is of size 2,6,etc, each a vector of size 169
                                // for a realization, we want to take 
                                for(size_t j=0;j!=s.size();++j){
                                        s[j] = S[j][cv[player_idx(j)]];
                                }
                                auto meta_result = combination_value(ctx, cv, s);
                                result(cv[player_idx(idx)]) += _.prob * meta_result[player_idx(idx)];
                        }
                } else {
                        for(holdem_class_perm_iterator iter(ctx.num_players()),end;iter!=end;++iter){

                                auto const& cv = *iter;
                                auto p = cv.prob();
                                // create a view of the vector, nothing fancy
                                for(size_t idx=0;idx!=s.size();++idx){
                                        s[idx] = S[idx][cv[idx]];
                                }
                                auto meta_result = combination_value(ctx, cv, s);
                                result(cv[idx]) += p * meta_result[idx];
                        }
                }

                return result;
        }
        Eigen::VectorXd choose_push_fold(Eigen::VectorXd const& push, Eigen::VectorXd const& fold){
                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }
                return result;
        }
        Eigen::VectorXd clamp(Eigen::VectorXd s){
                for(holdem_class_id idx=0;idx!=169;++idx){
                        s(idx) = ( s(idx) < .5 ? .0 : 1. );
                }
                return s;
        }
        
        Eigen::VectorXd unilateral_maximal_exploitable(gt_context const& ctx, size_t idx, std::vector<Eigen::VectorXd> const& S)
        {

                enum{ Dp = 4 };
                enum{ Debug = 0};
                static Eigen::VectorXd fold_s = Eigen::VectorXd::Zero(169);
                static Eigen::VectorXd push_s = Eigen::VectorXd::Ones(169);
                if(Debug) std::cout << "============== idx = " << idx << " =====================\n";
                auto copy = S;
                copy[idx] = push_s;
                auto push = unilateral_detail(ctx, idx, copy);
                if(Debug) pretty_print_strat(push, Dp);

                copy[idx] = fold_s;
                auto fold = unilateral_detail(ctx, idx, copy);
                if(Debug) pretty_print_strat(fold, Dp);

                return choose_push_fold(push, fold);
        }





        

        struct solver{
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx,
                                                          std::vector<Eigen::VectorXd> const& state)=0;
        };
        struct maximal_exploitable_solver_uniform : solver{
                explicit maximal_exploitable_solver_uniform(double factor = 0.05):factor_{factor}{}
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx,
                                                          std::vector<Eigen::VectorXd> const& state)override
                {
                        std::vector<Eigen::VectorXd> result(state.size());
                        
                        //for(size_t idx=0;idx!=state.size();++idx){
                        for(size_t idx=state.size();idx!=0;){
                                --idx;

                                auto counter = unilateral_maximal_exploitable(ctx,idx, state);
                                result[idx] = state[idx] * ( 1.0 - factor_ ) + counter * factor_;
                        }
                        return result;
                }
        private:
                double factor_;
        };
        struct maximal_exploitable_solver_uniform_mt : solver{
                explicit maximal_exploitable_solver_uniform_mt(double factor = 0.05):factor_{factor}{}
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx,
                                                          std::vector<Eigen::VectorXd> const& state)override
                {
                        boost::timer::auto_cpu_timer at;
                        using result_t = std::future<std::tuple<size_t, Eigen::VectorXd> >;
                        std::vector<result_t> tmp;
                        for(size_t idx=0;idx!=state.size();++idx){
                                auto fut = std::async(std::launch::async, [idx,&ctx,&state,this](){
                                        return std::make_tuple(idx,unilateral_maximal_exploitable(ctx,idx, state));
                                });
                                tmp.emplace_back(std::move(fut));
                        }
                        std::cout << "tmp.size() => " << tmp.size() << "\n"; // __CandyPrint__(cxx-print-scalar,tmp.size())
                        std::vector<Eigen::VectorXd> result(state.size());
                        for(auto& _ : tmp){
                                auto ret = _.get();
                                auto idx            = std::get<0>(ret);
                                auto const& counter = std::get<1>(ret);
                                result[idx] = state[idx] * ( 1.0 - factor_ ) + counter * factor_;
                        }
                        return result;
                }
        private:
                double factor_;
        };


        struct cond_single_strategy_lp1{
                using state_t = std::vector<Eigen::VectorXd>;
                cond_single_strategy_lp1(size_t idx, double epsilon)
                        :idx_(idx),
                        epsilon_(epsilon)
                {}
                bool operator()(state_t const& from, state_t const& to)const{
                        auto d = from[idx_] - to[idx_];
                        auto norm = d.lpNorm<1>();
                        auto cond = ( norm < epsilon_ );
                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)
                        return cond;
                }
        private:
                size_t idx_;
                double epsilon_;
        };

        struct make_solver{
                
                enum{ DefaultMaxIter = 400 };

                using state_t         = std::vector<Eigen::VectorXd>;
                using step_observer_t = std::function<void(state_t const&)>;
                using stoppage_condition_t = std::function<bool(state_t const&, state_t const&)>;

                explicit make_solver(gt_context const& ctx){
                        ctx_ = &ctx;
                }
                make_solver& use_solver(std::shared_ptr<solver> s){
                        solver_ = s;
                        return *this;
                }
                make_solver& max_steps(size_t n){
                        max_steps_ = n;
                        return *this;
                }
                make_solver& init_state(std::vector<Eigen::VectorXd> const& s0){
                        state0_ = s0;
                        return *this;
                }
                make_solver& observer(step_observer_t obs){
                        obs_.push_back(obs);
                        return *this;
                }
                make_solver& stoppage_condition(stoppage_condition_t cond){
                        stop_cond_ = cond;
                        return *this;
                }
                std::vector<Eigen::VectorXd> run(){

                        BOOST_ASSERT(ctx_ );
                        BOOST_ASSERT(solver_ );
                        BOOST_ASSERT(state0_.size() );
                        BOOST_ASSERT( stop_cond_ );

                        std::vector<Eigen::VectorXd> state = state0_;
                        for(auto& _ : obs_){
                                _(state);
                        }

                        for(size_t idx=0;idx<max_steps_;++idx){

                                auto next = solver_->step(*ctx_, state);

                                if( stop_cond_(state, next) ){
                                        state[0] = clamp(state[0]);
                                        state[1] = clamp(state[1]);
                                        return state;
                                }
                                state = next;
                                for(auto& _ : obs_){
                                        _(state);
                                }

                        }

                        std::vector<Eigen::VectorXd> result;
                        result.push_back(Eigen::VectorXd::Zero(169));
                        result.push_back(Eigen::VectorXd::Zero(169));

                        BOOST_LOG_TRIVIAL(warning) << "Failed to converge solve ctx = " << *ctx_;
                        return result;
                }
        private:
                gt_context const* ctx_;
                std::shared_ptr<solver> solver_;
                std::vector<Eigen::VectorXd> state0_;
                std::vector<step_observer_t > obs_;
                stoppage_condition_t stop_cond_;
                size_t max_steps_{DefaultMaxIter};
        };




} // end namespace gt

struct BetterHeadUpSolverCmd : Command{
        enum{ Debug = 1};
        enum{ Dp = 2 };
        explicit
        BetterHeadUpSolverCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                using namespace gt;
                //heads_up_description desc(0.5, 1, 10);
                three_player_description desc(0.5, 1, 10);

                auto state0 = desc.make_inital_state();

                auto state = state0;

                double factor = 0.05;

                for(auto ei=desc.begin_event(),ee=desc.end_event();ei!=ee;++ei){
                        std::cout << ei->to_string() << "\n";
                }

                for(;;){
                        auto next = state;
                        for(auto si=desc.begin_strategy(),se=desc.end_strategy();si!=se;++si){
                                auto fold_s = si->make_all_fold(state);
                                auto fold_ev = desc.expected_value_by_class_id(si->player_index(), fold_s);

                                auto push_s = si->make_all_push(state);
                                auto push_ev = desc.expected_value_by_class_id(si->player_index(), push_s);

                                auto counter= choose_push_fold(push_ev, fold_ev);


                                next[si->vector_index()] = state[si->vector_index()] * ( 1.0 - factor ) + counter * factor;

                        }

                        auto delta = next[0] - state[0];
                        auto norm = delta.lpNorm<1>();
                        if( norm < 0.1 )
                                break;

                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)

                        state = next;
                        if(Debug) pretty_print_strat(state[0], Dp);
                        if(Debug) pretty_print_strat(state[1], Dp);
                }
                for(auto& _ : state){
                        _ = clamp(_);
                        
                }
                for(auto si=desc.begin_strategy(),se=desc.end_strategy();si!=se;++si){
                        std::cout << si->description() << "\n";
                        pretty_print_strat(state[si->vector_index()], Dp);
                }
                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<BetterHeadUpSolverCmd> BetterHeadUpSolverCmdDecl{"better-heads-up-solver"};


struct HeadUpSolverCmd : Command{
        explicit
        HeadUpSolverCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;
	
                std::string cache_name{".cc.bin"};
                try{
                        cc.load(cache_name);
                }catch(std::exception const& e){
                        std::cerr << "Failed to load (" << e.what() << ")\n";
                        throw;
                }

                using namespace gt;

                size_t num_players = 2;

                // create a vector of num_players of zero vectors
                std::vector<Eigen::VectorXd> state0( ( num_players == 2 ? 2 : 6 ) , Eigen::VectorXd::Zero(169));
                for(auto& _ : state0){
                        _.fill(0.5);
                }
                

                using result_t = std::future<std::tuple<double, std::vector<Eigen::VectorXd> > >;
                std::vector<result_t> tmp;

                gt_context gtctx(num_players, 10, .5, 1.);

                #if 0
                hu_eval_tree{gtctx}.display();
                hu_eval_tree_flat{gtctx}.display();
                three_way_eval_tree{gtctx}.display();
                #endif


                auto solve = [&](auto num_players, auto eff){
                        gt_context gtctx(num_players, eff, .5, 1.);
                        std::shared_ptr<eval_tree_node> gt;
                        switch(num_players){
                        case 2:
                                gt = std::make_shared<hu_eval_tree_flat>(gtctx);
                                break;
                        case 3:
                                gt = std::make_shared<three_way_eval_tree>(gtctx);
                                break;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("unsupported"));
                        }
                        gt->display();
                        gtctx.use_game_tree(gt);

                        gtctx.use_cache(cc);
                        auto result = make_solver(gtctx)
                                .use_solver(std::make_shared<maximal_exploitable_solver_uniform_mt>())
                                .stoppage_condition(cond_single_strategy_lp1(0, 0.1))
                                .init_state(state0)
                                #if 1
                                .observer([](auto const& vec){
                                          static std::vector<std::string> v = { ""  , "p" , "f" , "pp", "pf", "fp" };
                                          for(size_t idx=0;idx!=vec.size();++idx){
                                                  std::cout << "--------" << v[idx] << "-------------\n";
                                                  pretty_print_strat(vec[idx], 2);
                                          }
                                })
                                #endif
                                .run();
                        return result;
                };

                auto enque = [&](double eff){
                        tmp.push_back(std::async([&,num_players, eff](){
                                auto result = solve(num_players, eff);
                                return std::make_tuple(eff, result);
                        }));
                };
                #if 0
                for(double eff = 5.0;eff <= 50.0;eff+=1){
                        enque(eff);
                }
                #else
                enque(10);
                #endif

                #if 1
                Eigen::VectorXd s0(169);
                s0.fill(.0);
                Eigen::VectorXd s1(169);
                s1.fill(.0);
                for(auto& _ : tmp){
                        auto aux = _.get();
                        auto eff = std::get<0>(aux);
                        auto const& vec = std::get<1>(aux);
                        for(size_t idx=0;idx!=169;++idx){
                                s0(idx) = std::max(s0(idx), eff*vec[0](idx));
                                s1(idx) = std::max(s1(idx), eff*vec[1](idx));
                        }
                }
                
                pretty_print_strat(s0, 1);
                pretty_print_strat(s1, 1);




                #if 0
                auto order_cards = [](auto const& strat){
                        struct HandAux{
                                HandAux(size_t id_, double level_)
                                        :id(id_),
                                        level(level_),
                                        decl{&holdem_hand_decl::get(id)}
                                {}
                                size_t id;
                                double level;
                                holdem_hand_decl const* decl;
                                double cum_{.0};
                        };
                        std::vector<HandAux> aux;
                        for(size_t idx=0;idx!=strat.size();++idx){
                                aux.emplace_back(idx, strat[idx]);
                        }
                        // first sort by level
                        std::sort( aux.begin(), aux.end(), [](auto const& l, auto const& r){
                                return l.level > r.level;
                        });
                        holdem_hand_vector result;
                        for(auto const& _ : aux){
                                result.push_back(_.id);
                        }
                        

                        return result;
                };
                #endif


                #endif

                #if 0
                for(auto& _ : tmp){
                        auto aux = _.get();
                        auto const& vec = std::get<1>(aux);
                        for(auto const& s : vec ){
                                pretty_print_strat(s, 2);
                        }
                }
                #endif



                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<HeadUpSolverCmd> HeadsUpSolverCmdDecl{"heads-up-solver"};
        
} // end namespace ps
