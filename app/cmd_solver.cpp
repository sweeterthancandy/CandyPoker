#ifndef PS_CMD_BETTER_SOLVER_H
#define PS_CMD_BETTER_SOLVER_H

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
        inline Eigen::VectorXd choose_push_fold(Eigen::VectorXd const& push, Eigen::VectorXd const& fold){
                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }
                return result;
        }
        inline Eigen::VectorXd clamp(Eigen::VectorXd s){
                for(holdem_class_id idx=0;idx!=169;++idx){
                        s(idx) = ( s(idx) < .5 ? .0 : 1. );
                }
                return s;
        }

        struct BetterHeadUpSolverCmd : Command{
                enum{ Debug = 1};
                enum{ Dp = 2 };
                explicit
                BetterHeadUpSolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        //heads_up_description desc(0.5, 1, 10);
                        three_player_description desc(0.5, 1, 10);

                        auto state0 = desc.make_inital_state();

                        auto state = state0;

                        double factor = 0.10;

                        for(auto ei=desc.begin_event(),ee=desc.end_event();ei!=ee;++ei){
                                std::cout << ei->to_string() << "\n";
                        }

                        auto step = [&](auto const& state)->binary_strategy_description::strategy_impl_t
                        {
                                boost::timer::auto_cpu_timer at;
                                using result_t = std::future<std::tuple<size_t, Eigen::VectorXd> >;
                                std::vector<result_t> tmp;
                                for(auto si=desc.begin_strategy(),se=desc.end_strategy();si!=se;++si){
                                        auto fut = std::async(std::launch::async, [&,si](){
                                                auto fold_s = si->make_all_fold(state);
                                                auto fold_ev = desc.expected_value_by_class_id(si->player_index(), fold_s);

                                                auto push_s = si->make_all_push(state);
                                                auto push_ev = desc.expected_value_by_class_id(si->player_index(), push_s);

                                                auto counter= choose_push_fold(push_ev, fold_ev);
                                                return std::make_tuple(si->vector_index(), counter);
                                        });
                                        tmp.emplace_back(std::move(fut));
                                }
                                auto result = state;
                                for(auto& _ : tmp){
                                        auto ret = _.get();
                                        auto idx            = std::get<0>(ret);
                                        auto const& counter = std::get<1>(ret);
                                        result[idx] = state[idx] * ( 1.0 - factor ) + counter * factor;
                                }
                                return result;
                        };
                        
                        auto print = [&]()
                        {
                                for(auto si=desc.begin_strategy(),se=desc.end_strategy();si!=se;++si){
                                        std::cout << si->description() << "\n";
                                        pretty_print_strat(state[si->vector_index()], Dp);
                                }
                        };

                        for(;;){
                                auto next = step(state);

                                auto delta = next[0] - state[0];
                                auto norm = delta.lpNorm<1>();
                                if( norm < 0.1 )
                                        break;

                                std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)

                                state = next;
                                if(Debug) print();
                        }
                        for(auto& _ : state){
                                _ = clamp(_);
                                
                        }
                        print();
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<BetterHeadUpSolverCmd> BetterHeadUpSolverCmdDecl{"better-heads-up-solver"};
} // end namespace ps

#endif // PS_CMD_BETTER_SOLVER_H
