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
#include "ps/eval/binary_strategy_description.h"


namespace ps{

        #if 0
        struct dev_class_cache_item{
                holdem_class_vector cv;
                std::vector<double> ev;
        };
        inline
        size_t hash_value(dev_class_cache_item const& item){
                return boost::hash_range(item.cv.begin(), item.cv.end());
        }
        #endif


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

        struct counter_strategy_concept{
                virtual ~counter_strategy_concept()=default;
                virtual Eigen::VectorXd counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state)const=0;
        };
        struct counter_strategy_elementwise : counter_strategy_concept{
                virtual Eigen::VectorXd counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state)const override
                {
                        auto fold_s = decl.make_all_fold(state);
                        auto push_s = decl.make_all_push(state);

                        Eigen::VectorXd counter(169);
                        counter.fill(0);
                        for(holdem_class_id class_idx=0;class_idx!=169;++class_idx){
                                auto fold_ev = strategy_desc.expected_value_for_class_id(decl.player_index(),
                                                                                 class_idx,
                                                                                 fold_s);
                                auto push_ev = strategy_desc.expected_value_for_class_id(decl.player_index(),
                                                                                 class_idx,
                                                                                 push_s);
                                double val = ( fold_ev <= push_ev ? 1.0 : 0.0 );
                                counter[class_idx] = val;
                        }
                        return counter;
                }
        };
        struct counter_strategy_aggresive : counter_strategy_concept{
                struct Context{
                        Eigen::VectorXd& counter;
                        binary_strategy_description const& strategy_desc;
                        binary_strategy_description::strategy_decl const& decl;
                        binary_strategy_description::strategy_impl_t const& state;
                        binary_strategy_description::strategy_impl_t fold_s;
                        binary_strategy_description::strategy_impl_t push_s;
                        std::set<holdem_class_id> __check;
                        size_t derived_counter{0};
                };
                struct Op{
                        explicit Op(holdem_class_id cid):cid_{cid}, decl_{holdem_class_decl::get(cid_)}{}
                        virtual void push_or_fold(Context& ctx)const{
                                do_push_or_fold(ctx);
                        }
                        virtual std::string to_string()const{
                                std::stringstream sstr;
                                sstr << "Op{" << decl_ << "}";
                                return sstr.str();
                        }
                protected:
                        double do_push_or_fold_impl(Context& ctx)const
                        {
                                auto fold_ev = ctx.strategy_desc.expected_value_for_class_id(ctx.decl.player_index(),
                                                                                 cid_,
                                                                                 ctx.fold_s);
                                auto push_ev = ctx.strategy_desc.expected_value_for_class_id(ctx.decl.player_index(),
                                                                                 cid_,
                                                                                 ctx.push_s);
                                double val = ( fold_ev <= push_ev ? 1.0 : 0.0 );
                                return val;
                        }
                        void do_push_or_fold(Context& ctx)const
                        {
                                ctx.counter[cid_] = do_push_or_fold_impl(ctx);
                                ctx.__check.insert(cid_);
                        }
                        holdem_class_id cid_;
                        holdem_class_decl const& decl_;
                };
                struct Any : Op{
                        enum{ Debug = 0 };
                        explicit Any(holdem_class_id cid):Op{cid}{}
                        virtual void push_or_fold(Context& ctx)const override{
                                for(auto derived_cid : derived_true_){
                                        if( ctx.__check.count(derived_cid) == 0){
                                                std::cout << "derived_cid => " << (int)derived_cid << "\n"; // __CandyPrint__(cxx-print-scalar,derived_cid)
                                                holdem_class_vector tmp(ctx.__check.begin(), ctx.__check.end());
                                                std::cout << "tmp => " << tmp << "\n"; // __CandyPrint__(cxx-print-scalar,tmp)
                                                throw std::domain_error("bad order");
                                        }
                                        if( ctx.counter[derived_cid] == 1.0 ){
                                                if( Debug ){
                                                        if( do_push_or_fold_impl(ctx) != 1.0 ){
                                                                std::cout << to_string() << "\n";
                                                                throw std::domain_error("bad map");
                                                        }
                                                }
                                                ctx.counter[cid_] = 1.0;
                                                ++ctx.derived_counter;
                                                ctx.__check.insert(cid_);
                                                if( Debug ){
                                                        std::cout << "derived " << holdem_class_decl::get(cid_) 
                                                                << " from " << holdem_class_decl::get(derived_cid ) << "\n";
                                                }
                                                return;
                                        }
                                }
                                for(auto derived_cid : derived_false_){
                                        if( ctx.__check.count(derived_cid) == 0){
                                                std::cout << "derived_cid => " << (int)derived_cid << "\n"; // __CandyPrint__(cxx-print-scalar,derived_cid)
                                                holdem_class_vector tmp(ctx.__check.begin(), ctx.__check.end());
                                                std::cout << "tmp => " << tmp << "\n"; // __CandyPrint__(cxx-print-scalar,tmp)
                                                throw std::domain_error("bad order");
                                        }
                                        if( ctx.counter[derived_cid] == 0.0 ){
                                                if( Debug ){
                                                        if( do_push_or_fold_impl(ctx) != 0.0 ){
                                                                std::cout << to_string() << "\n";
                                                                throw std::domain_error("bad map");
                                                        }
                                                }
                                                ctx.counter[cid_] = 0.0;
                                                ctx.__check.insert(cid_);
                                                ++ctx.derived_counter;
                                                if( Debug ){
                                                        std::cout << "derived " << holdem_class_decl::get(cid_) 
                                                                << " from " << holdem_class_decl::get(derived_cid ) << "\n";
                                                }
                                                return;
                                        }
                                }
                                do_push_or_fold(ctx);
                        }
                        virtual std::string to_string()const override{
                                std::stringstream sstr;
                                sstr << "Any{" << decl_ << ", true=" << derived_true_ << ", false=" << derived_false_ << "}";
                                return sstr.str();
                        }
                        void take_true(holdem_class_id cid){
                                derived_true_.push_back(cid);
                        }
                        void take_false(holdem_class_id cid){
                                derived_false_.push_back(cid);
                        }
                private:
                        holdem_class_vector derived_true_;
                        holdem_class_vector derived_false_;
                };
                counter_strategy_aggresive(){
                        // pocket pairs
                        
                        for(size_t A=0;A!=13;++A){
                                auto cid = holdem_class_decl::make_id(holdem_class_type::pocket_pair, A, A);
                                #if 0
                                std::cout << "A => " << A << "\n"; // __CandyPrint__(cxx-print-scalar,A)
                                std::cout << "holdem_class_decl::get(cid) => " << holdem_class_decl::get(cid) << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_class_decl::get(cid))
                                #endif
                                auto op = std::make_shared<Any>(cid);
                                ops_.push_back(op);
                                #if 1
                                if( A != 0 ){
                                        auto prev = holdem_class_decl::make_id(holdem_class_type::pocket_pair,A-1,A-1);
                                        op->take_true(prev);
                                }
                                #endif
                        }
                        
                        // suited
                        for(size_t A=13;A!=0;){
                                --A;
                                for(size_t B=0;B!=A;++B){
                                        auto cid = holdem_class_decl::make_id(holdem_class_type::suited, A, B);
                                        auto op = std::make_shared<Any>(cid);
                                        ops_.push_back(op);

                                        #if 0
                                        std::cout << "A => " << A << "\n"; // __CandyPrint__(cxx-print-scalar,A)
                                        std::cout << "B => " << B << "\n"; // __CandyPrint__(cxx-print-scalar,B)
                                        std::cout << "holdem_class_decl::get(cid) => " << holdem_class_decl::get(cid) << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_class_decl::get(cid))
                                        #endif

                                        #if 0
                                        if( A != 12 ){
                                                if( B != 0 ){
                                                        auto lower_kicker = holdem_class_decl::make_id(holdem_class_type::suited, A, B-1);
                                                        op->take_true(lower_kicker);
                                                }
                                        }
                                        #endif

                                }
                        }

                        // offsuit
                        for(size_t A=13;A!=0;){
                                --A;
                                for(size_t B=0;B!=A;++B){

                                        auto cid = holdem_class_decl::make_id(holdem_class_type::offsuit, A, B);
                                        auto op = std::make_shared<Any>(cid);
                                        ops_.push_back(op);

                                        #if 1
                                        auto suited_id = holdem_class_decl::make_id(holdem_class_type::suited, A, B);
                                        op->take_false(suited_id);
                                        #endif
                                }
                        }

                        for(auto const& _ : ops_){
                                std::cout << _->to_string() << "\n";
                        }
                }
                virtual Eigen::VectorXd counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state)const override
                {
                        Eigen::VectorXd counter(169);
                        counter.fill(0);
                        Context ctx = {
                                counter, strategy_desc, decl, state, 
                                decl.make_all_fold(state), decl.make_all_push(state)
                        };
                        for(auto const& _ : ops_){
                                _->push_or_fold(ctx);
                        }
                        if( ctx.__check.size() != 169 )
                                throw std::domain_error("bad mapping");
                        std::cout << "ctx.derived_counter => " << ctx.derived_counter << "\n"; // __CandyPrint__(cxx-print-scalar,ctx.derived_counter)
                        return counter;
                }
        private:
                std::vector<std::shared_ptr<Op> > ops_;
        };

        struct SolverCmd : Command{
                enum{ Debug = 1};
                enum{ Dp = 2 };
                explicit
                SolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        std::unique_ptr<binary_strategy_description> desc;
                        std::unique_ptr<counter_strategy_concept> cptr(new counter_strategy_aggresive);

                        if( args_.size() && args_[0] == "three"){
                                desc = binary_strategy_description::make_three_player_description(0.5, 1, 20);
                        } else{
                                desc = binary_strategy_description::make_hu_description(0.5, 1, 20);
                        }


                        auto state0 = desc->make_inital_state();

                        auto state = state0;

                        double factor = 0.10;


                        using namespace Pretty;
                        std::vector<LineItem> lines;
                        size_t line_idx = 0;
                        std::vector<std::string> header;
                        header.push_back("n");
                        for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                header.push_back(si->action());
                        }
                        header.push_back("max");
                        for(size_t idx=0;idx!=desc->num_players();++idx){
                                std::stringstream sstr;
                                sstr << "ev[" << idx << "]";
                                header.push_back(sstr.str());
                        }
                        header.push_back("time");
                        lines.push_back(std::move(header));
                        lines.push_back(LineBreak);

                        boost::timer::cpu_timer timer;


                        using state_type = binary_strategy_description::strategy_impl_t;

                        auto step = [&](auto const& state)->state_type
                        {
                                using result_t = std::future<std::tuple<size_t, Eigen::VectorXd> >;
                                std::vector<result_t> tmp;
                                for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                        auto fut = std::async(std::launch::async, [&,si](){
                                                return std::make_tuple(si->vector_index(), cptr->counter(*desc, *si, state));
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
                        

                        auto stoppage_condition = [&](state_type const& from, state_type const& to)->bool
                        {
                                std::vector<double> norm_vec;
                                for(size_t idx=0;idx!=from.size();++idx){
                                        auto delta = from[idx] - to[idx];
                                        auto norm = delta.lpNorm<1>();
                                        norm_vec.push_back(norm);
                                }
                                auto norm = *std::max_element(norm_vec.begin(),norm_vec.end());
                                
                                std::vector<std::string> line;
                                line.push_back(boost::lexical_cast<std::string>(line_idx));
                                for(size_t idx=0;idx!=norm_vec.size();++idx){
                                        line.push_back(boost::lexical_cast<std::string>(norm_vec[idx]));
                                }

                                
                                line.push_back(boost::lexical_cast<std::string>(norm));
                                auto ev = desc->expected_value(to);
                                for(size_t idx=0;idx!=desc->num_players();++idx){
                                        line.push_back(boost::lexical_cast<std::string>(ev[idx]));
                                }
                                line.push_back(format(timer.elapsed(), 2, "%w(%t cpu)"));
                                lines.push_back(std::move(line));
                                ++line_idx;

                                RenderTablePretty(std::cout, lines);

                                double epsilon = 0.05;
                                return norm < epsilon;
                        };
                        
                        auto print = [&]()
                        {
                                for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                        std::cout << si->description() << "\n";
                                        pretty_print_strat(state[si->vector_index()], Dp);
                                }
                        };

                        for(;;){
                                timer.start();
                                auto next = step(state);
                                timer.stop();

                                if( stoppage_condition(state, next) )
                                        break;

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
        static TrivialCommandDecl<SolverCmd> SolverCmdDecl{"solver"};
} // end namespace ps

#endif // PS_CMD_BETTER_SOLVER_H
