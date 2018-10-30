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
                enum MaybeBool{
                        MB_False,
                        MB_True,
                        MB_Unknown,
                };
                struct Context{
                        enum{ Debug = false };
                        binary_strategy_description const& strategy_desc;
                        binary_strategy_description::strategy_decl const& decl;
                        binary_strategy_description::strategy_impl_t const& state;
                        binary_strategy_description::strategy_impl_t fold_s;
                        binary_strategy_description::strategy_impl_t push_s;
                        std::set<holdem_class_id> __check;
                        size_t derived_counter{0};
                        std::array<MaybeBool, 169> result;

                        Context( binary_strategy_description const& strategy_desc_,
                                 binary_strategy_description::strategy_decl const& decl_,
                                 binary_strategy_description::strategy_impl_t const& state_)
                                : strategy_desc(strategy_desc_)
                                , decl(decl_)
                                , state(state_)
                                , fold_s( decl.make_all_fold(state) )
                                , push_s( decl.make_all_push(state) )
                        {
                                result.fill(MB_Unknown);
                        }

                        Eigen::VectorXd Counter()const{
                                Eigen::VectorXd counter{169};
                                for(size_t idx=0;idx!=169;++idx){
                                        switch(result[idx]){
                                        case MB_True:
                                                counter[idx] = 1.0;
                                                break;
                                        case MB_False:
                                                counter[idx] = 0.0;
                                                break;
                                        case MB_Unknown:
                                                throw std::domain_error("not defined");
                                        }
                                }
                                return counter;
                        }
                        MaybeBool GetMaybeValue(holdem_class_id cid)const{
                                return result[cid];
                        }
                        double GetRealValue(holdem_class_id cid)const{
                                switch(result[cid]){
                                case MB_True:
                                        return 1.0;
                                case MB_False:
                                        return 0.0;
                                default:
                                        throw std::domain_error("cid not set");
                                }
                        }
                        MaybeBool GetMaybeValueOrCompute(holdem_class_id cid){
                                switch(result[cid]){
                                        case MB_True:
                                                return MB_True;
                                        case MB_False:
                                                return MB_False;
                                        case MB_Unknown:
                                        {
                                                auto ret = UnderlyingComputation(cid);
                                                SetValue(cid, ret);
                                                return ret;
                                        }
                                }
                                PS_UNREACHABLE();
                        }
                        void SetValue(holdem_class_id cid, MaybeBool val){
                                if( result[cid] != MB_Unknown)
                                        throw std::domain_error("cid already sett");
                                assert( val != MB_Unknown && "precondition failed");
                                result[cid] = val;
                        }
                        void TakeDerived(holdem_class_id cid, MaybeBool val){
                                if( Debug ){
                                        if( result[cid] != MB_Unknown )
                                                throw std::domain_error("cid already set");
                                        auto real_val = UnderlyingComputation(cid);
                                        if( real_val != val ){
                                                std::stringstream sstr;
                                                sstr << "bad val " << real_val << " <> " << val;
                                                throw std::domain_error(sstr.str());
                                        }
                                }
                                SetValue(cid, val);
                        }
                        MaybeBool UnderlyingComputation(holdem_class_id cid)const
                        {
                                auto fold_ev = strategy_desc.expected_value_for_class_id(decl.player_index(),
                                                                                         cid,
                                                                                         fold_s);
                                auto push_ev = strategy_desc.expected_value_for_class_id(decl.player_index(),
                                                                                         cid,
                                                                                         push_s);
                                return ( fold_ev <= push_ev ? MB_True : MB_False );
                        }

                };
                struct Op{
                        virtual void push_or_fold(Context& ctx)const=0;
                        virtual std::string to_string()const=0;
                };
                struct Row : Op, holdem_class_vector{
                        virtual void push_or_fold(Context& ctx)const override{
                                // we just start at the front untill we find one that is zero
                                auto first_false_idx = [&](){
                                        size_t idx=0;
                                        for(;idx!=size();++idx){
                                                auto cid = at(idx);
                                                if( ctx.GetMaybeValueOrCompute(cid) == MB_False )
                                                        return idx;
                                        }
                                        return idx;
                                }();
                                for(;first_false_idx!=size();++first_false_idx){
                                        auto cid = at(first_false_idx);
                                        ctx.TakeDerived(cid, MB_False);
                                }
                        }
                        virtual std::string to_string()const{
                                std::stringstream sstr;
                                sstr << "Row{" << *this << "}";
                                return sstr.str();
                        }
                };
                struct Any : Op{
                        enum{ Debug = 1 };
                        explicit Any(holdem_class_id cid):cid_{cid}, decl_{holdem_class_decl::get(cid_)}{}
                        virtual void push_or_fold(Context& ctx)const override{
                                for(auto derived_cid : derived_true_){
                                        if(ctx.GetMaybeValue(derived_cid) == MB_True){
                                                ctx.TakeDerived(cid_, MB_True);
                                                return;
                                        }
                                }
                                for(auto derived_cid : derived_false_){
                                        if(ctx.GetMaybeValue(derived_cid) == MB_False){
                                                ctx.TakeDerived(cid_, MB_False);
                                                return;
                                        }
                                }
                                ctx.SetValue(cid_, ctx.UnderlyingComputation(cid_));
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
                        holdem_class_id cid_;
                        holdem_class_decl const& decl_;
                        holdem_class_vector derived_true_;
                        holdem_class_vector derived_false_;
                };
                counter_strategy_aggresive(){
                        // pocket pairs
                        
                        for(size_t A=0;A!=13;++A){
                                auto cid = holdem_class_decl::make_id(holdem_class_type::pocket_pair, A, A);
                                auto op = std::make_shared<Any>(cid);
                                ops_.push_back(op);
                                if( A != 0 ){
                                        auto prev = holdem_class_decl::make_id(holdem_class_type::pocket_pair,A-1,A-1);
                                        op->take_true(prev);
                                }
                        }
                        
                        // suited
                        for(size_t A=13;A!=0;){
                                --A;
                                for(size_t B=0;B!=A;++B){
                                        auto cid = holdem_class_decl::make_id(holdem_class_type::suited, A, B);
                                        auto op = std::make_shared<Any>(cid);
                                        ops_.push_back(op);
                                }
                        }

                        // offsuit
                        for(size_t A=13;A!=0;){
                                --A;
                                for(size_t B=0;B!=A;++B){

                                        auto cid = holdem_class_decl::make_id(holdem_class_type::offsuit, A, B);
                                        auto op = std::make_shared<Any>(cid);
                                        ops_.push_back(op);
#if 0

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
                        Context ctx(strategy_desc, decl, state);
                        for(auto const& _ : ops_){
                                _->push_or_fold(ctx);
                        }
                        return ctx.Counter();
                }
        private:
                std::vector<std::shared_ptr<Op> > ops_;
        };

        #if 0
        struct counter_strategy_smart : counter_strategy_concept{
                struct Row : std::vector<holdem_class_id>{
                };
                virtual Eigen::VectorXd counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state)const override
                {
                        /*
                                The idea is that for any strategy, we ALWAYS have the constaruct that 
                                the top-left to bottom-right diagonal is monotonic. 

                                                   A K Q J T 9 8 7 6 5 4 3 2 
                                                  +--------------------------
                                                A |1 1 1 1 1 1 1 1 1 1 1 1 1
                                                K |1 1 1 1 1 1 1 1 1 1 1 1 1
                                                Q |1 1 1 1 1 1 1 1 0 0 0 0 0
                                                J |1 1 1 1 1 1 1 0 0 0 0 0 0
                                                T |1 1 1 1 1 1 0 0 0 0 0 0 0
                                                9 |1 1 1 0 0 1 0 0 0 0 0 0 0
                                                8 |1 1 0 0 0 0 1 0 0 0 0 0 0
                                                7 |1 1 0 0 0 0 0 1 0 0 0 0 0
                                                6 |1 1 0 0 0 0 0 0 1 0 0 0 0
                                                5 |1 1 0 0 0 0 0 0 0 1 0 0 0
                                                4 |1 0 0 0 0 0 0 0 0 0 1 0 0
                                                3 |1 0 0 0 0 0 0 0 0 0 0 1 0
                                                2 |1 0 0 0 0 0 0 0 0 0 0 0 1

                                This strategy is that we want, for each diagonal on the offsuit side, 
                                we want to choose which is the point where either 
                                                The right end is One => all One
                                                The left end is Zero => all zero
                                                X is One and  X+1 is Zero 

                                        ( A2 )
                                        ( A3, K2 )
                                        ( A4, K3, Q2 )
                                        ( A5, K4, Q3, J2 )
                                        ( A6, K5, Q4, J3, T2 )
                                        ( A7, K6, Q5, J4, T3, 92 )
                                        ( A8, K7, Q6, J5, T4, 93, 82 )
                                        ( A9, K8, Q7, J6, T5, 94, 83, 72 )
                                        ( AT, K9, Q8, J7, T6, 95, 84, 73, 62 )
                                        ( AJ, KT, Q9, J8, T7, 96, 85, 74, 63, 52 )
                                        ( AQ, KJ, QT, J9, T8, 97, 86, 75, 64, 53, 42)
                                        ( AK, KQ, QJ, JT, T9, 98, 87, 76, 65, 54, 43, 32)



                         */
                }
        };
        #endif
        struct SolverCmd : Command{
                enum{ Debug = 0};
                enum{ Dp = 2 };
                explicit
                SolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        std::unique_ptr<binary_strategy_description> desc;
                        std::unique_ptr<counter_strategy_concept> cptr(new counter_strategy_aggresive);

                        if( args_.size() && args_[0] == "three"){
                                desc = binary_strategy_description::make_three_player_description(0.5, 1, 10);
                        } else{
                                desc = binary_strategy_description::make_hu_description(0.5, 1, 10);
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
