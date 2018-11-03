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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>



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
                enum{ Debug = false };
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
                                                {
                                                        std::stringstream sstr;
                                                        sstr << "not defined " << holdem_class_decl::get(idx);
                                                        throw std::domain_error(sstr.str());
                                                }
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
                                if( result[cid] != MB_Unknown){
                                        std::cout << "holdem_class_decl::get(cid) => " << holdem_class_decl::get(cid) << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_class_decl::get(cid))
                                        throw std::domain_error("cid already sett");
                                }
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
                                ++derived_counter;
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
                        virtual MaybeBool Compute(Context& ctx, holdem_class_id cid)const{
                                return ctx.GetMaybeValueOrCompute(cid);
                        }
                        virtual void push_or_fold(Context& ctx)const override{
                                // we just start at the front untill we find one that is zero
                                auto start = [&](){
                                        size_t idx=0;
                                        for(;idx!=size();++idx){
                                                auto cid = at(idx);
                                                if( Compute(ctx, cid) == MB_False )
                                                        return idx + 1;
                                        }
                                        return idx;
                                }();
                                for(;start!=size();++start){
                                        auto cid = at(start);
                                        ctx.TakeDerived(cid, MB_False);
                                }
                        }
                        virtual std::string to_string()const{
                                std::stringstream sstr;
                                sstr << "Row{" << *this << "}";
                                return sstr.str();
                        }
                };
                struct SuitedRow : Row{
                        virtual MaybeBool Compute(Context& ctx, holdem_class_id cid)const override{
                                auto const& decl = holdem_class_decl::get(cid);
                                auto offsuit_cid = holdem_class_decl::make_id(holdem_class_type::offsuit, decl.first(), decl.second());
                                switch(ctx.GetMaybeValue(offsuit_cid)){
                                        case MB_True:
                                        {
                                                // if the offsuit card is true, then flush must be true
                                                ctx.TakeDerived(cid, MB_True);
                                                return MB_True;
                                        }
                                        // if the offsuit card is false, then maybe
                                        case MB_False:
                                        case MB_Unknown:
                                        {
                                                auto ret = ctx.UnderlyingComputation(cid);
                                                ctx.SetValue(cid, ret);
                                                return ret;
                                        }
                                }
                                PS_UNREACHABLE();
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
                        
                        #if 0
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
                        #endif

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
                        auto pp = std::make_shared<Row>();
                        for(size_t A=13;A!=0;){
                                --A;
                                pp->push_back( holdem_class_decl::make_id(holdem_class_type::pocket_pair, A, A));
                        }
                        ops_.push_back(pp);

                        for(size_t d = 1; d != 13; ++d){
                                auto ptr = std::make_shared<Row>();
                                for(size_t A=13;A!=0;){
                                        --A;
                                        if( A >= d ){
                                                ptr->push_back( holdem_class_decl::make_id(holdem_class_type::offsuit, A, A-d));
                                        }
                                }
                                ops_.push_back(ptr);
                        }

                        for(size_t d = 1; d != 13; ++d){
                                auto ptr = std::make_shared<SuitedRow>();
                                for(size_t A=13;A!=0;){
                                        --A;
                                        if( A >= d ){
                                                ptr->push_back( holdem_class_decl::make_id(holdem_class_type::suited, A, A-d));
                                        }
                                }
                                ops_.push_back(ptr);
                        }


                        if( Debug ){
                                for(auto const& _ : ops_){
                                        std::cout << _->to_string() << "\n";
                                }
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

        // basically a wrapper class
        struct holdem_binary_strategy{
                holdem_binary_strategy()=default;
                /* implicit */ holdem_binary_strategy(std::vector<Eigen::VectorXd> const& state){
                        for(auto const& vec : state){
                                state_.emplace_back();
                                state_.back().resize(vec.size());
                                for(size_t idx=0;idx!=vec.size();++idx){
                                        state_.back()[idx] = vec[idx];
                                }
                        }
                }
                std::vector<Eigen::VectorXd> to_eigen()const{
                        std::vector<Eigen::VectorXd> tmp;
                        for(auto const& v : state_){
                                Eigen::VectorXd ev(v.size());
                                for(size_t idx=0;idx!=v.size();++idx){
                                        ev[idx] = v[idx];
                                }
                                tmp.push_back(std::move(ev));
                        }
                        return tmp;
                }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & state_;
                }
        private:
                std::vector< std::vector<double> > state_;
        };

        template<class ImplType>
        struct serialization_base{
                void load(std::string const& path){
                        using archive_type = boost::archive::text_iarchive;
                        std::ifstream ofs(path);
                        archive_type oa(ofs);
                        oa >> *reinterpret_cast<ImplType*>(this);
                }
                void save(std::string const& path)const {
                        using archive_type = boost::archive::text_oarchive;
                        std::ofstream ofs(path);
                        archive_type oa(ofs);
                        oa << *reinterpret_cast<ImplType const*>(this);
                }
        };

        struct holdem_binary_strategy_ledger : serialization_base<holdem_binary_strategy_ledger>{
                void push(holdem_binary_strategy s){
                        ledger_.emplace_back(std::move(s));
                }
                auto size()const{ return ledger_; }
                auto const& back()const{ return ledger_.back(); }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & ledger_;
                }
        private:
                std::vector<holdem_binary_strategy> ledger_;
        };

        struct holdem_binary_solution_set : serialization_base<holdem_binary_solution_set>{
                void add_solution(double eff, holdem_binary_strategy solution){
                        solutions_.emplace_back(eff, std::move(solution));
                }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & solutions_;
                }
        private:
                std::vector<std::pair<double, holdem_binary_strategy> > solutions_;
        };


        struct Continue{};
        struct Break{};
        struct Error{ std::string msg; };

        using  holdem_binary_solver_ctrl = boost::variant<Continue, Break, Error>;

        struct holdem_binary_solver;

        struct holdem_binary_solver_any_observer{
                using state_type = binary_strategy_description::strategy_impl_t;
                virtual ~holdem_binary_solver_any_observer()=default;
                virtual holdem_binary_solver_ctrl start(holdem_binary_solver const*, state_type const& state){ return Continue{}; }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const*, state_type const& from, state_type const& to){ return Continue{}; }
                virtual holdem_binary_solver_ctrl finish(holdem_binary_solver const*, state_type const& state){ return Continue{}; }
                virtual void imbue(holdem_binary_solver* solver){}
        };

        struct holdem_binary_solver_result{
                using state_type = binary_strategy_description::strategy_impl_t;
                state_type state;
                size_t n{0};
                holdem_binary_solver_ctrl stop_condition;
                
                bool success()const{
                        return ( boost::get<Break>(&stop_condition) != nullptr );
                }
        };
        struct holdem_binary_solver{
                using state_type = binary_strategy_description::strategy_impl_t;
                void use_description(std::shared_ptr<binary_strategy_description> desc){
                        desc_ = desc;
                }
                void use_strategy(std::shared_ptr<counter_strategy_concept> counter_strat){
                        counter_strategy_ = counter_strat;
                }
                void add_observer(std::shared_ptr<holdem_binary_solver_any_observer> obs){
                        obs_.push_back(obs);
                }
                void use_inital_state(state_type state){
                        initial_state_ = std::move(state);
                }
                holdem_binary_solver_result compute(){
                        static double const factor = 0.10;
                        
                        for(auto& _ : obs_ ){
                                _->imbue(this);
                        }

                        auto step = [&](auto const& state)->state_type
                        {
                                using result_t = std::future<std::tuple<size_t, Eigen::VectorXd> >;
                                std::vector<result_t> tmp;
                                for(auto si=desc_->begin_strategy(),se=desc_->end_strategy();si!=se;++si){
                                        auto fut = std::async(std::launch::async, [&,si](){
                                                return std::make_tuple(si->vector_index(), counter_strategy_->counter(*desc_, *si, state));
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
                        
                        state_type state;

                        if( initial_state_ ){
                                state = *initial_state_;
                        } else {
                                state = desc_->make_inital_state();
                        }
                        


                        for(auto& _ : obs_ ){
                                _->start(this, state);
                        }

                        size_t n = 0;
                        for(;;++n){
                                auto next = step(state);

                                for(auto& _ : obs_ ){
                                        auto result = _->step(this, state, next);
                                        if( auto ptr = boost::get<Continue>(&result)){
                                                continue;
                                        }
                                        if( auto ptr = boost::get<Break>(&result)){
                                                return { state, n, result };
                                        }
                                        if( auto ptr = boost::get<Error>(&result)){
                                                return { state, n, result };
                                        }
                                        PS_UNREACHABLE();
                                }

                                state = next;
                        }
                        PS_UNREACHABLE();
                }
                std::shared_ptr<binary_strategy_description> get_description()const{
                        return desc_;
                }
        private:
                boost::optional<state_type> initial_state_;
                // desripbes the game
                std::shared_ptr<binary_strategy_description> desc_;
                // computes the coutner strategy
                std::shared_ptr<counter_strategy_concept> counter_strategy_;

                // all the observers
                std::vector<std::shared_ptr<holdem_binary_solver_any_observer> > obs_;
        };

        struct table_observer : holdem_binary_solver_any_observer{
                table_observer(binary_strategy_description* desc)
                        : desc_{desc}
                {
                        using namespace Pretty;
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

                        timer.start();
                }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const*, state_type const& from, state_type const& to)override{

                        std::vector<std::string> norm_vec_s;
                        std::vector<double> norm_vec;

                        for(size_t idx=0;idx!=from.size();++idx){
                                auto delta = from[idx] - to[idx];
                                auto norm = delta.lpNorm<1>();
                                norm_vec.push_back(norm);
                        }
                        auto norm = *std::max_element(norm_vec.begin(),norm_vec.end());

                        for(auto val : norm_vec){
                                norm_vec_s.push_back(boost::lexical_cast<std::string>(val));
                        }
                        norm_vec_s.push_back(boost::lexical_cast<std::string>(norm));


                        using namespace Pretty;
                        std::vector<std::string> line;
                        line.push_back(boost::lexical_cast<std::string>(n_));
                        for(size_t idx=0;idx!=norm_vec_s.size();++idx){
                                line.push_back(boost::lexical_cast<std::string>(norm_vec_s[idx]));
                        }

                        
                        auto ev = desc_->expected_value(to);
                        for(size_t idx=0;idx!=desc_->num_players();++idx){
                                line.push_back(boost::lexical_cast<std::string>(ev[idx]));
                        }
                        line.push_back(format(timer.elapsed(), 2, "%w(%t cpu)"));
                        timer.start();
                        lines.push_back(std::move(line));
                        RenderTablePretty(std::cout, lines);
                        ++n_;
                        last_ = to;
                        return Continue{};
                }
        private:
                binary_strategy_description* desc_;
                size_t n_{0};
                std::vector<Pretty::LineItem> lines;
                boost::timer::cpu_timer timer;
                boost::optional<state_type> last_;
        };
        
        struct lp_inf_stoppage_condition : holdem_binary_solver_any_observer{
                explicit lp_inf_stoppage_condition(double epsilon = 0.05)
                        : epsilon_{epsilon}
                {}
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const*, state_type const& from, state_type const& to)override{
                        std::vector<double> norm_vec;
                        for(size_t idx=0;idx!=from.size();++idx){
                                auto delta = from[idx] - to[idx];
                                auto norm = delta.lpNorm<1>();
                                norm_vec.push_back(norm);
                        }
                        auto norm = *std::max_element(norm_vec.begin(),norm_vec.end());

                        bool cond = norm < epsilon_;
                        if( cond )
                                return Break{};
                        return Continue{};
                }
        private:
                double epsilon_;
        };
        struct max_steps_condition : holdem_binary_solver_any_observer{
                explicit max_steps_condition(size_t n):n_{n}{}
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        if( ++count_ > n_){
                                std::stringstream sstr;
                                sstr << "Too many steps (" << n_ << ") for " << solver->get_description()->string_representation();
                                return Error{sstr.str()};
                        }
                        return Continue{};
                }
        private:
                size_t n_;
                size_t count_{0};
        };
        struct strategy_printer : holdem_binary_solver_any_observer{
                enum{ Dp = 2 };
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        auto desc = solver->get_description();
                        for(auto si=desc->begin_strategy(),se=desc->end_strategy();si!=se;++si){
                                std::cout << si->description() << "\n";
                                pretty_print_strat(to[si->vector_index()], Dp);
                        }
                        return Continue{};
                }
        private:
                binary_strategy_description* desc_;
        };

        struct solver_ledger : holdem_binary_solver_any_observer{
                explicit solver_ledger(std::string const& filename)
                        : filename_{filename}
                {}
                virtual holdem_binary_solver_ctrl start(holdem_binary_solver const*, state_type const& state)override{
                        if( ! skip_start_ ){
                                ledger_.push(state);
                                ledger_.save(filename_);
                                ledger_.save(filename_ + ".other");
                        }
                        return Continue{};
                }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const*, state_type const& from, state_type const& to)override{
                        ledger_.push(to);
                        ledger_.save(filename_);
                        ledger_.save(filename_ + ".other");
                        return Continue{};
                }
                virtual void imbue(holdem_binary_solver* solver)override{
                        try{
                                ledger_.load(filename_);
                                auto state0 = ledger_.back().to_eigen();
                                solver->use_inital_state(std::move(state0));
                        }catch(...){
                                skip_start_ = false;
                        }
                }
        private:
                std::string filename_;
                bool skip_start_{true};
                holdem_binary_strategy_ledger ledger_;
        };

        

        struct SolverCmd : Command{
                enum{ Debug = 1};
                explicit
                SolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{


                        std::shared_ptr<binary_strategy_description> desc;
                        std::string ledger_path;
                        if( args_.size() && args_[0] == "three"){
                                desc = binary_strategy_description::make_three_player_description(0.5, 1, 10);
                                ledger_path = ".3_player_ledger.bin";
                        } else{
                                desc = binary_strategy_description::make_hu_description(0.5, 1, 10);
                                ledger_path = ".2_player_ledger.bin";
                        }

                        holdem_binary_solver solver;
                        solver.use_description(desc);
                        solver.use_strategy(std::make_shared<counter_strategy_aggresive>());
                        solver.add_observer(std::make_shared<lp_inf_stoppage_condition>());
                        solver.add_observer(std::make_shared<table_observer>(desc.get()));
                        solver.add_observer(std::make_shared<solver_ledger>(ledger_path));
                        solver.add_observer(std::make_shared<strategy_printer>());

                        auto result = solver.compute();
                        if( result.success() ){
                                for(auto& _ : result.state){
                                        _ = clamp(_);
                                }
                        }

                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<SolverCmd> SolverCmdDecl{"solver"};
        
        
        struct hu_table_maker{
                using state_type = binary_strategy_description::strategy_impl_t;
                template<class FutureMaker>
                static state_type make_table(FutureMaker&& solver){
                        using result_t = std::future<std::tuple<double, holdem_binary_solver_result > >;
                        std::vector<result_t> tmp;
                        for(double eff=2;eff!=20;eff+=0.1){
                                tmp.emplace_back(std::async([&,e=eff](){
                                        return std::make_tuple(e, solver(e));
                                }));
                        }
                        state_type state;
                        
                        for(auto& _ : tmp){
                                auto aux = _.get();
                                auto eff        = std::get<0>(aux);
                                auto const& ret = std::get<1>(aux);
                                if( ! ret.success() ){
                                        if( auto ptr = boost::get<Error>(&ret.stop_condition)){
                                                std::cerr << "Error: " << ptr->msg << "\n";
                                        }
                                        continue;
                                }
                                auto const& vec = ret.state;
                                for(; state.size() < vec.size();){
                                        state.emplace_back(169);
                                        state.back().fill(0.0);
                                }
                                for(size_t i=0;i!=vec.size();++i){
                                        for(size_t cid=0;cid!=169;++cid){
                                                if( state[i][cid] < eff*vec[i][cid] ){
                                                        state[i][cid] = eff*vec[i][cid];
                                                }
                                        }
                                }
                        }
                        return state;
                }
        };
        struct HuTable : Command{
                enum{ Debug = 1};
                explicit
                HuTable(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{

                        enum{ MaxSteps = 400 };

                        auto maker = [](double eff)
                        {
                                auto desc = binary_strategy_description::make_hu_description(0.5, 1, eff);
                                auto desc_sptr = std::shared_ptr<binary_strategy_description>(desc.get(), [](auto _){});

                                holdem_binary_solver solver;
                                solver.use_description(desc_sptr);
                                solver.use_strategy(std::make_shared<counter_strategy_aggresive>());
                                solver.add_observer(std::make_shared<lp_inf_stoppage_condition>());
                                solver.add_observer(std::make_shared<max_steps_condition>(MaxSteps));
                                auto result = solver.compute();
                                if( result.success()){
                                        for(auto& _ : result.state){
                                                _ = clamp(_);
                                        }
                                }
                                return result;
                        };
                        auto table = hu_table_maker::make_table(maker);
                        for(auto const& s : table){
                                pretty_print_strat(s, 0);
                        }


                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<HuTable> HuTableDecl{"hu-table"};
} // end namespace ps

#endif // PS_CMD_BETTER_SOLVER_H
