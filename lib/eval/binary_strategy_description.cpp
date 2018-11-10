#include "ps/eval/binary_strategy_description.h"
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

namespace ps{



        struct static_event : binary_strategy_description::event_decl{
                explicit static_event(std::string const& key, Eigen::VectorXd vec):
                        key_{key}, vec_{vec}
                {}
                virtual std::string key()const override{ return key_; }
                virtual void expected_value_given_event(Eigen::VectorXd& out, holdem_class_vector const& cv, double p)const override{
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
                eval_event( binary_strategy_description::eval_view* eval, 
                            std::string const& key,
                           std::vector<size_t> perm,
                           Eigen::VectorXd const& dead_money,
                           Eigen::VectorXd const& active)
                        :eval_{eval}, key_(key), perm_{perm}, dead_money_{dead_money}, active_{active}
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
                virtual void expected_value_given_event(Eigen::VectorXd& out, holdem_class_vector const& cv, double p)const override{

                        // short circuit for optimization purposes
                        if( std::fabs(p) < 0.001 )
                                return;

                        std::array<size_t, 9> re_perm;
                        std::copy(perm_.begin(), perm_.end(), re_perm.begin());
                        auto re_perm_end = re_perm.begin() + perm_.size();
                        std::sort(re_perm.begin(), re_perm_end, [&](size_t l, size_t r){
                                return cv[l] < cv[r];
                        });


                        holdem_class_vector tmp;
                        for(size_t idx=0;idx!=perm_.size();++idx){
                                tmp.push_back(cv[re_perm[idx]]);
                        }
                        
                        #if 0
                        auto cc = Memory_ClassCache.get();
                        #endif
                        #if 0
                        static hash_class_cache cc_;
                        static auto* cc = &cc_;
                        #endif
                        auto ev_ptr = eval_->eval_no_perm(tmp);
                        auto const& ev = *ev_ptr;
                        
                        out += delta_proto_ * p;

                        size_t ev_idx = 0;
                        for(size_t idx=0;idx!=perm_.size();++idx){
                                out[re_perm[idx]] += pot_amt_ * ev[ev_idx] * p;
                                ++ev_idx;
                        }
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
                binary_strategy_description::eval_view* eval_;
                std::string key_;
                std::vector<size_t> perm_;
                Eigen::VectorXd dead_money_; // not used
                Eigen::VectorXd active_; // not used
                Eigen::VectorXd delta_proto_;
                double pot_amt_;
        };


        struct heads_up_description : binary_strategy_description{
                heads_up_description(eval_view* eval, double sb, double bb, double eff)
                        : sb_{sb}, bb_{bb}, eff_{eff}
                {
                        eval_ = eval;



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
                        auto n_pp = std::make_shared<eval_event>(eval_, "pp", std::vector<size_t>{0,1}, dead_money, active);
                        events_.push_back(n_pp);

                        strats_.emplace_back(this, 0,0, "SB Pushing", "");
                        strats_.emplace_back(this, 1,1, "BB Calling, given a SB push", "p");
                        
                        finish();
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
                virtual size_t strat_vector_size()const{ return 2; }
                
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
                        for(auto const& group : *Memory_TwoPlayerClassVector){
                                for(auto const& _ : group.vec){
                                        auto const& cv = _.cv;
                                        auto ev = expected_value_of_vector(aux_event_set_, cv, impl);
                                        result(cv[player_idx]) += _.prob * ev[player_idx];
                                }
                        }
                        return result;
                }
                virtual Eigen::VectorXd expected_value(strategy_impl_t const& impl)const override{
                        Eigen::VectorXd result(2);
                        result.fill(0);
                        for(auto const& group : *Memory_TwoPlayerClassVector){
                                for(auto const& _ : group.vec){
                                        auto const& cv = _.cv;
                                        check_probability_of_event(cv, impl);
                                        auto ev = expected_value_of_vector(aux_event_set_, cv, impl);
                                        result[0] += _.prob * ev[0];
                                        result[1] += _.prob * ev[1];
                                }
                        }
                        return result;
                }
                virtual double expected_value_for_class_id_es(event_set const& es, size_t player_idx, holdem_class_id class_id, strategy_impl_t const& impl)const override{
                        double result = 0.0;
                        holdem_class_vector cv;
                        for(auto const& group : *Memory_TwoPlayerClassVector){
                                if( group.cid != class_id)
                                        continue;
                                for(auto const& _ : group.vec){
                                        cv = _.cv;
                                        if( player_idx != 0 )
                                                std::swap(cv[0], cv[player_idx]);
                                        auto ev = expected_value_of_vector(es, cv, impl);
                                        result += _.prob * ev[player_idx];
                                }
                        }
                        return result;
                }
        private:
                double sb_;
                double bb_;
                double eff_;
        };
        
        
        struct three_player_description : binary_strategy_description{
                three_player_description(eval_view* eval, double sb, double bb, double eff)
                        : sb_{sb}, bb_{bb}, eff_{eff}
                {
                        eval_ = eval;


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

                                        auto allin = std::make_shared<eval_event>(eval_, key, perm, dead_money, active);
                                        events_.push_back(allin);
                                }
                                
                        }

                        strats_.emplace_back(this, 0,0, "BTN Pushing"                        , ""  );
                        strats_.emplace_back(this, 1,1, "SB Calling, given BTN Push"         , "p" );
                        strats_.emplace_back(this, 2,1, "SB Pushing, given BTN Fold"         , "f" ); 
                        strats_.emplace_back(this, 3,2, "BB Calling, given BTN Push, SB Call", "pp");
                        strats_.emplace_back(this, 4,2, "BB Calling, given BTN Push, SB Fold", "pf");
                        strats_.emplace_back(this, 5,2, "BB Calling, given BTN Fold, SB Push", "fp");
        
                        finish();
                }
                virtual strategy_impl_t make_inital_state()const override{
                        Eigen::VectorXd proto(169);
                        proto.fill(.5);
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
                virtual size_t strat_vector_size()const{ return 6; }
                
                /*
                         The index of the strategy vector, for example
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
                        for(auto const& group : *Memory_ThreePlayerClassVector){
                                for(auto const& _ : group.vec){
                                        auto const& cv = _.cv;
                                        auto ev = expected_value_of_vector(aux_event_set_, cv, impl);
                                        result(cv[player_idx]) += _.prob * ev[player_idx];
                                }
                        }
                        return result;
                }
                virtual Eigen::VectorXd expected_value(strategy_impl_t const& impl)const override{
                        Eigen::VectorXd result(3);
                        result.fill(0);
                        for(auto const& group : *Memory_ThreePlayerClassVector){
                                for(auto const& _ : group.vec){
                                        auto const& cv = _.cv;
                                        auto ev = expected_value_of_vector(aux_event_set_, cv, impl);
                                        result[0] += _.prob * ev[0];
                                        result[1] += _.prob * ev[1];
                                        result[2] += _.prob * ev[2];
                                }
                        }
                        return result;
                }
                virtual double expected_value_for_class_id_es(event_set const& es, size_t player_idx, holdem_class_id class_id, strategy_impl_t const& impl)const override{
                        double result = 0.0;
                        holdem_class_vector cv;
                        for(auto const& group : *Memory_ThreePlayerClassVector){
                                if( group.cid != class_id)
                                        continue;
                                for(auto const& _ : group.vec){
                                        cv = _.cv;
                                        if( player_idx != 0 )
                                                std::swap(cv[0], cv[player_idx]);
                                        auto ev = expected_value_of_vector(es, cv, impl);
                                        result += _.prob * ev[player_idx];
                                }
                        }
                        return result;
                }
        private:
                double sb_;
                double bb_;
                double eff_;
        };
                
        std::shared_ptr<binary_strategy_description> binary_strategy_description::make_hu_description(eval_view* eval, double sb, double bb, double eff){
                return std::make_shared<heads_up_description>(eval, sb, bb, eff);
        }
        std::shared_ptr<binary_strategy_description> binary_strategy_description::make_three_player_description(eval_view* eval, double sb, double bb, double eff){
                return std::make_shared<three_player_description>(eval, sb, bb, eff);
        }

} // end namespace ps
