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

#include "ps/support/persistent_impl.h"


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
                /*
                        This is represents the abstract function of figurting out that, given everyone
                        eles's strategy is constant, should we push or fold each class.
                                Now to allow optmizations, we take as an argument the last result,
                        where the state should only be slightly permuated. However this is optional,
                        and a bad hint don't effect performance.
                        
                 */
                virtual Eigen::VectorXd produce_counter(binary_strategy_description const& strategy_desc,
                                                        binary_strategy_description::strategy_decl const& decl,
                                                        binary_strategy_description::strategy_impl_t const& state,
                                                        boost::optional<Eigen::VectorXd> hint)const=0;
        };
        struct counter_strategy_elementwise_batch : counter_strategy_concept{
                enum{ Debug = false };
                virtual Eigen::VectorXd produce_counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state,
                                                        boost::optional<Eigen::VectorXd> hint)const override
                {
                        auto fold_s = decl.make_all_fold(state);
                        auto push_s = decl.make_all_push(state);
                                        

                        auto fold_ev = strategy_desc.expected_value_by_class_id(decl.player_index(), fold_s);
                        auto push_ev = strategy_desc.expected_value_by_class_id(decl.player_index(), push_s);
                        

                        Eigen::VectorXd counter(169);
                        counter.fill(0);
                        for(holdem_class_id cid=0;cid!=169;++cid){
                                double val = ( fold_ev[cid] <= push_ev[cid] ? 1.0 : 0.0 );
                                counter[cid] = val;
                        }

                        if( Debug ){
                                static std::mutex mtx;
                                std::lock_guard<std::mutex> lock(mtx);
                                enum{ Dp = 8 };
                                std::cout << "-------------- " << strategy_desc.string_representation() << " " << decl.description() << "\n";
                                std::cout << "fold_s\n";
                                pretty_print_strat(fold_s[decl.vector_index()], Dp);
                                std::cout << "push_s\n";
                                pretty_print_strat(push_s[decl.vector_index()], Dp);
                                std::cout << "fold_ev\n";
                                pretty_print_strat(fold_ev, Dp);
                                std::cout << "push_ev\n";
                                pretty_print_strat(push_ev, Dp);
                                auto delta = push_ev - fold_ev;
                                std::cout << "<delta>\n";
                                pretty_print_strat(delta, Dp);
                                std::cout << "counter\n";
                                pretty_print_strat(counter, Dp);
                        }
                        
                        return counter;
                }
        };
        struct counter_strategy_elementwise : counter_strategy_concept{
                virtual Eigen::VectorXd produce_counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state,
                                                        boost::optional<Eigen::VectorXd> hint)const override
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
                enum{ Debug = true };
                enum MaybeBool{
                        MB_False,
                        MB_True,
                        MB_Unknown,
                };
                struct Context{
                        enum{ Debug = true };
                        binary_strategy_description const& strategy_desc;
                        binary_strategy_description::strategy_decl const& decl;
                        binary_strategy_description::strategy_impl_t const& state;
                        boost::optional<Eigen::VectorXd> hint;
                        binary_strategy_description::strategy_impl_t fold_s;
                        binary_strategy_description::strategy_impl_t push_s;
                        size_t derived_counter{0};
                        std::array<MaybeBool, 169> result;

                        Context( binary_strategy_description const& strategy_desc_,
                                 binary_strategy_description::strategy_decl const& decl_,
                                 binary_strategy_description::strategy_impl_t const& state_,
                                 boost::optional<Eigen::VectorXd> const& hint_)
                                : strategy_desc(strategy_desc_)
                                , decl(decl_)
                                , state(state_)
                                , hint(hint_)
                                , fold_s( decl.make_all_fold(state) )
                                , push_s( decl.make_all_push(state) )
                        {
                                result.fill(MB_Unknown);
                        }
                        static std::string mb_to_string(MaybeBool mb){
                                return ( mb == MB_True  ? "True"   :
                                         mb == MB_False ? "False"  :
                                                          "Unknown" );
                        }

                        void CheckAll()const{
                                holdem_class_vector false_push;
                                holdem_class_vector false_fold;
                                for(size_t idx=0;idx!=169;++idx){
                                        auto mb = result[idx];
                                        if(mb == MB_Unknown )
                                                continue;
                                        auto check = UnderlyingComputation(idx);
                                        if( check == mb )
                                                continue;
                                        if( mb == MB_True )
                                                false_push.push_back(idx);
                                        else
                                                false_fold.push_back(idx);
                                }
                                if( false_push.size() + false_fold.size()){
                                        std::stringstream sstr;
                                        sstr << "fold_push=" << false_push << ", false_fold=" << false_fold;
                                        throw std::domain_error(sstr.str());
                                }
                        }
                        void Check(std::string const& from, size_t idx, MaybeBool mb)const{
                                auto check = UnderlyingComputation(idx);
                                #if 0
                                for(size_t idx=0;idx!=13;++idx){
                                        std::cout << "idx => " << idx << "\n"; // __CandyPrint__(cxx-print-scalar,idx)
                                        std::cout << "holdem_class_decl::get(idx) => " << holdem_class_decl::get(idx) << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_class_decl::get(idx))
                                }
                                #endif
                                if( mb != check ){
                                        std::stringstream sstr;
                                        sstr << "bad check [" << from << "]at " <<  holdem_class_decl::get(idx) 
                                             << " real=" << mb_to_string(check)
                                             << " vs " << mb_to_string(mb);
                                        //std::cout << "sstr.str() => " << sstr.str() << "\n"; // __CandyPrint__(cxx-print-scalar,sstr.str())
                                        throw std::domain_error(sstr.str());
                                }
                        }
                        Eigen::VectorXd Counter()const{
                                Eigen::VectorXd counter{169};
                                for(size_t idx=0;idx!=169;++idx){
                                        switch(result[idx]){
                                                case MB_True:
                                                        if( Debug )
                                                                Check("Counter()", idx, result[idx]);
                                                        counter[idx] = 1.0;
                                                        break;
                                                case MB_False:
                                                        if( Debug )
                                                                Check("Counter()", idx, result[idx]);
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
                                        Check("TakeDerived()", cid, val);
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
                                #if 0
                                std::cout << "[" << holdem_class_decl::get(cid) << "] ";
                                std::cout << "fold_ev => " << fold_ev << "\n"; // __CandyPrint__(cxx-print-scalar,fold_ev)
                                #endif
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
                                #if 0
                                auto start = [&](){
                                        size_t idx=0;
                                        for(;idx!=size();++idx){
                                                auto cid = at(idx);
                                                if( Compute(ctx, cid) == MB_False )
                                                        return idx + 1;
                                        }
                                        return idx;
                                }();
                                #endif
                                auto start = find_bounary(ctx);
                                #if 0
                                std::cerr << *this << " : " << "start => " << start << "\n"; // __CandyPrint__(cxx-print-scalar,start)
                                if( start != size() ){
                                        std::cerr << "start=" << holdem_class_decl::get(start) << "\n";
                                } else {
                                        std::cerr << "start=END\n";
                                }
                                std::cerr << "doing checks\n";
                                if( start != 0 ){
                                        ctx.Check("Row<T>()" + sstr.str(), at(start-1), MB_True);
                                }
                                if( start != size() ){
                                        ctx.Check("Row<F>()" + sstr.str(), at(start), MB_False);
                                }
                                std::cerr << "ending checks\n";
                                #endif


                                for(size_t idx=start;idx!=size();++idx){
                                        auto cid = at(idx);
                                        if(ctx.GetMaybeValue(cid) == MB_Unknown ){
                                                ctx.TakeDerived(cid, MB_False);
                                        }
                                }
                                for(size_t idx=0;idx!=start;++idx){
                                        auto cid = at(idx);
                                        if(ctx.GetMaybeValue(cid) == MB_Unknown ){
                                                ctx.TakeDerived(cid, MB_True);
                                        }
                                }
                                #if 0
                                for(auto cid : *this){
                                        if(ctx.GetMaybeValue(cid) == MB_Unknown )
                                                throw std::domain_error("wtf");
                                }
                                ctx.CheckAll();
                                #endif
                        }
                        virtual std::string to_string()const{
                                std::stringstream sstr;
                                sstr << "Row{" << *this << "}";
                                return sstr.str();
                        }
                        // returns the index one after the first item in the row
                        // for which is false, or the end of the sequence otherwise
                        //
                        //
                        // \return \in [1,size()]
                        //  0 => 0
                        //  1 => 10
                        //  2 => 110
                        //  ...
                        //  size()-1 =>  11..10
                        //  size()   => 11...1
                        size_t find_bounary(Context& ctx)const{
                                double epsilon = 1e-5;
                                size_t idx = 0;

                                if( ctx.hint ){
                                        auto const& hint = *ctx.hint;
                                        for(; idx+1<size();++idx){
                                                auto cid = at(idx);
                                                if( std::fabs(hint[cid]) < epsilon ){
                                                        // we have found the boundry
                                                        break;
                                                }
                                        }
                                }

                                if( Compute(ctx, at(idx)) == MB_True ){
                                        // if we have one that is true, we want to find the first false one
                                        ++idx;
                                        for(; idx!= size(); ++idx){
                                                if( Compute(ctx, at(idx)) == MB_False ){
                                                        return idx;
                                                }
                                        }
                                        return size();
                                } else {
                                        // else we have found a fold, we want to find the first true one
                                        for(;idx!=0;){
                                                --idx;
                                                if( Compute(ctx, at(idx)) == MB_True){
                                                        return idx+1;
                                                }
                                        }
                                        // if we get here, they are all false
                                        return 0;
                                }
                                PS_UNREACHABLE();
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
                counter_strategy_aggresive(){

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

                                With this parametrization, we don't have to worry that A5 is better than A6 sometimes, because 
                                they are in different rows



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
                virtual Eigen::VectorXd produce_counter(binary_strategy_description const& strategy_desc,
                                                binary_strategy_description::strategy_decl const& decl,
                                                binary_strategy_description::strategy_impl_t const& state,
                                                boost::optional<Eigen::VectorXd> hint)const override
                {
                        Eigen::VectorXd counter(169);
                        counter.fill(0);
                        Context ctx(strategy_desc, decl, state, hint);
                        for(auto const& _ : ops_){
                                _->push_or_fold(ctx);
                        }
                        if( Debug ){
                                std::cout << "ctx.derived_counter => " << ctx.derived_counter << "\n"; // __CandyPrint__(cxx-print-scalar,ctx.derived_counter)
                        }
                        std::cerr << "Making\n";
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
                        std::lock_guard<std::mutex> lock(mtx_);
                        using archive_type = boost::archive::text_iarchive;
                        std::ifstream ofs(path);
                        archive_type oa(ofs);
                        oa >> *reinterpret_cast<ImplType*>(this);
                        path_ = path;
                }
                // returns true indicte that load from disk
                // returns false to indicate that an empty object represention
                //         was created and loaded from disk
                bool try_load_or_default(std::string const& path){
                        try{
                                load(path);
                                return true;
                        }catch(...){
                                // clear it
                                auto typed = reinterpret_cast<ImplType*>(this);
                                typed->~ImplType();
                                new(typed)ImplType;
                                // now write to disk
                                save_as(path);
                                // no load it again so any error is apprent now
                                load(path);
                                return false;
                        }
                }
                void save_as(std::string const& path)const {
                        std::lock_guard<std::mutex> lock(mtx_);
                        using archive_type = boost::archive::text_oarchive;
                        std::ofstream ofs(path);
                        archive_type oa(ofs);
                        oa << *reinterpret_cast<ImplType const*>(this);
                }
                void save_()const{
                        if( path_.size() ){
                                save_as(path_);
                        }
                }
        private:
                mutable std::mutex mtx_;
                std::string path_;
        };

        struct holdem_binary_strategy_ledger : serialization_base<holdem_binary_strategy_ledger>{
                void push(holdem_binary_strategy s){
                        ledger_.emplace_back(std::move(s));
                }
                size_t size()const{ return ledger_.size(); }
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
                void add_solution(std::string const& key, holdem_binary_strategy solution){
                        std::lock_guard<std::mutex> lock(mtx_);
                        solutions_.emplace(key, std::move(solution));
                }
                // there are no thread safe
                auto begin()const{ return solutions_.begin(); }
                auto end()const{ return solutions_.end(); }
                auto find(std::string const& key)const{ return solutions_.find(key); }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & solutions_;
                }
        private:
                std::mutex mtx_;
                // in our scope we can represent everyting in a string, ie
                // we might want to encode the sb:bb:eff:stragery etc here
                std::map<std::string, holdem_binary_strategy> solutions_;
        };


        struct Continue{};
        struct Break{ std::string msg; };
        struct Error{ std::string msg; };
        struct SmallerFactor{};

        using  holdem_binary_solver_ctrl = boost::variant<Continue, Break, Error, SmallerFactor>;

        struct holdem_binary_solver;

        struct holdem_binary_solver_any_observer{
                using state_type = binary_strategy_description::strategy_impl_t;
                explicit holdem_binary_solver_any_observer(std::string const& name):name_{name}{}
                virtual ~holdem_binary_solver_any_observer()=default;
                virtual holdem_binary_solver_ctrl start(holdem_binary_solver const*, state_type const& state){ return Continue{}; }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const*, state_type const& from, state_type const& to){ return Continue{}; }
                virtual holdem_binary_solver_ctrl finish(holdem_binary_solver const*, state_type const& state){ return Continue{}; }
                virtual void imbue(holdem_binary_solver* solver){}

                std::string const& get_name()const{ return name_; }

                virtual size_t precedence()const{ return 10; }
        private:
                std::string name_;
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
                        obs->imbue(this);
                        obs_.push_back(obs);
                }
                template<class T>
                T const* get_observer()const{
                        for(auto const& _ : obs_){
                                if( _->get_name() == T::static_get_name()){
                                        return reinterpret_cast<T const*>(_.get());
                                }
                        }
                        return nullptr;
                }
                // this is intended to be used for the Imbue section
                template<class T>
                void use_observer(){
                        auto ptr = this->get_observer<T>();
                        if( ptr == nullptr){
                                add_observer( std::make_shared<T>() );
                        }
                }
                void use_inital_state(state_type state){
                        initial_state_ = std::move(state);
                }
                holdem_binary_solver_result compute(){

                        std::stable_sort(obs_.begin(), obs_.end(), [](auto const& r, auto const& l){
                                return r->precedence() < l->precedence();
                        });

                        std::vector<boost::optional<Eigen::VectorXd> > hint_vector(desc_->strat_vector_size());

                        auto step = [&](auto const& state)->state_type
                        {
                                struct Task{
                                        binary_strategy_description::strategy_decl const* sd;
                                        Eigen::VectorXd solution;
                                };
                                using result_t = std::future<Task>;
                                std::vector<result_t> tmp;
                                for(auto si=desc_->begin_strategy(),se=desc_->end_strategy();si!=se;++si){
                                        auto fut = std::async(std::launch::async, [&,sd=*si](){
                                                auto sol = counter_strategy_->produce_counter(*desc_,
                                                                                              sd,
                                                                                              state,
                                                                                              hint_vector[sd.vector_index()]);
                                                hint_vector[sd.vector_index()] = sol;
                                                return Task{&sd, std::move(sol)};
                                        });
                                        tmp.emplace_back(std::move(fut));
                                }
                                auto result = state;
                                for(auto& _ : tmp){
                                        auto ret = _.get();
                                        auto idx            = ret.sd->vector_index();
                                        auto const& counter = ret.solution;
                                        std::cout << "vector index is " << idx << "\n";
                                        result[idx] = state[idx] * ( 1.0 - factor_ ) + counter * factor_;
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

                        auto finish = [&](auto const& result){
                                for(auto& _ : obs_ ){
                                        _->finish(this, result);
                                }
                        };

                        size_t n = 0;
                        for(;;++n){
                                auto next = step(state);

                                for(auto& _ : obs_ ){
                                        auto result = _->step(this, state, next);
                                        if( auto ptr = boost::get<Continue>(&result)){
                                                continue;
                                        }
                                        if( auto ptr = boost::get<Break>(&result)){
                                                finish(state);
                                                return { state, n, result };
                                        }
                                        if( auto ptr = boost::get<Error>(&result)){
                                                finish(state);
                                                return { state, n, result };
                                        }
                                        if( auto ptr = boost::get<SmallerFactor>(&result)){
                                                std::cerr << "changing factor\n";
                                                factor_ /= 2.0;
                                                continue;
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

                double factor_{0.10};
        };

        struct table_observer : holdem_binary_solver_any_observer{
                table_observer(binary_strategy_description* desc, bool print_step = false)
                        : holdem_binary_solver_any_observer{"table_observer"}, desc_{desc}, print_step_{print_step}
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
                        ++n_;
                        last_ = to;

                        // don't want this for hu
                        if( print_step_ ){
                                RenderTablePretty(std::cout, lines);
                        }

                        return Continue{};
                }
                virtual holdem_binary_solver_ctrl finish(holdem_binary_solver const*, state_type const& state)override{

                        RenderTablePretty(std::cout, lines);
                        return Continue{};
                }
        private:
                binary_strategy_description* desc_;
                bool print_step_;
                size_t n_{0};
                std::vector<Pretty::LineItem> lines;
                boost::timer::cpu_timer timer;
                boost::optional<state_type> last_;
        };
        
        struct lp_inf_stoppage_condition : holdem_binary_solver_any_observer{
                explicit lp_inf_stoppage_condition(double epsilon = 0.005)
                        : holdem_binary_solver_any_observer{"lp_inf_stoppage_condition"}
                        , epsilon_{epsilon}
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
                                return Break{"lp_inf_stoppage_condition"};
                        return Continue{};
                }
        private:
                double epsilon_;
        };
        // for this stoppage condtiion, we want that the ev is less than norm
        struct ev_diff_stoppage_condition : holdem_binary_solver_any_observer{
                explicit ev_diff_stoppage_condition(double epsilon = 0.000001, size_t stride = 1)
                        : holdem_binary_solver_any_observer{"ev_diff_stoppage_condition"}
                        , epsilon_{epsilon}, stride_{stride}
                {
                        assert( stride != 0 );
                }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        if( ++count_ % stride_ != 0 )
                                return Continue{};
                        auto desc = solver->get_description();
                        auto from_ev = desc->expected_value(from);
                        auto to_ev = desc->expected_value(to);

                        auto delta = from_ev - to_ev;
                        auto norm = delta.lpNorm<1>();
                        auto cond = ( norm < epsilon_ );
                        if( cond ){
                                std::stringstream sstr;
                                sstr << std::fixed;
                                sstr << "ev_diff_stoppage_condition: norm=" << norm << ", epsilon=" << epsilon_;
                                return Break{sstr.str()};
                        }
                        return Continue{};
                }
        private:
                double epsilon_;
                size_t stride_;
                size_t count_{0};
        };
        struct ev_seq : holdem_binary_solver_any_observer{
                static std::string static_get_name(){ return "ev_seq"; }
                ev_seq() : holdem_binary_solver_any_observer{static_get_name()}{}
                struct min_max_type{
                        double min_;
                        double max_;

                        double delta()const{ return max_ - min_; }
                };

                virtual void imbue(holdem_binary_solver* solver)override{
                        n_ = solver->get_description()->num_players();
                }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        auto desc = solver->get_description();
                        auto to_ev = desc->expected_value(to);
                        seq_.push_back(to_ev);
                        return Continue{};
                }
                boost::optional<std::vector<min_max_type> > make_min_max(size_t lookback)const{
                        Eigen::VectorXd min_(n_), max_(n_);
                        min_.fill(+DBL_MAX);;
                        max_.fill(-DBL_MAX);
                        if(seq_.size() < lookback )
                                return boost::none;
                        for(size_t idx=lookback; idx!=0;){
                                --idx;
                                auto const& v = seq_[seq_.size()-1-idx];
                                for(size_t j=0;j!=v.size();++j){
                                        if( v[j] < min_[j]){
                                                min_[j] = v[j];
                                        }
                                        if( v[j] > max_[j]){
                                                max_[j] = v[j];
                                        }
                                }
                        }
                        std::vector<min_max_type> result;
                        for(size_t idx=0;idx!=n_;++idx){
                                result.emplace_back( min_max_type{ min_[idx], max_[idx] } );
                        }
                        return result;
                }
                virtual size_t precedence()const override{ return 0; }
        private:
                size_t n_{0};
                std::vector< Eigen::VectorXd > seq_;
        };

        struct ev_seq_printer : holdem_binary_solver_any_observer{
                ev_seq_printer(): holdem_binary_solver_any_observer{"ev_seq_printer"}{}
                virtual void imbue(holdem_binary_solver* solver)override{
                        solver->use_observer<ev_seq>();
                }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        enum{ Lookback = 100 };

                        auto ptr = solver->get_observer<ev_seq>();
                        if( ! ptr )
                                return Continue{};

                        auto opt = ptr->make_min_max(Lookback);
                        if( ! opt )
                                return Continue{};
                        table_.push_back(std::move(*opt));

                        using namespace Pretty;
                        std::vector<LineItem> lines;
                        lines.push_back(std::vector<std::string>{"range[0]", "delta[0]", "range[1]", "delta[1]"});
                        lines.push_back(LineBreak);
                        for(auto const& row : table_){
                                std::vector<std::string> line;
                                for(auto const& col : row ){
                                        std::stringstream sstr;
                                        sstr << std::fixed << std::setprecision(10);
                                        sstr << "(" << col.min_ << "," << col.max_ << ")";
                                        line.emplace_back(sstr.str());
                                        sstr.str("");
                                        sstr << col.max_ - col.min_;
                                        line.emplace_back(sstr.str());
                                }
                                lines.push_back(std::move(line));
                        }
                        RenderTablePretty(std::cout, lines);
                        return Continue{};

                }
                std::vector< std::vector< ev_seq::min_max_type > > table_;
        };

        struct ev_seq_break : holdem_binary_solver_any_observer{
                explicit ev_seq_break(double epsilon): holdem_binary_solver_any_observer{"ev_seq_break"}, epsilon_{epsilon}{}
                virtual void imbue(holdem_binary_solver* solver)override{
                        solver->use_observer<ev_seq>();
                }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        enum{ Lookback = 100 };

                        auto ptr = solver->get_observer<ev_seq>();
                        if( ! ptr )
                                return Continue{};

                        auto opt = ptr->make_min_max(Lookback);
                        if( ! opt )
                                return Continue{};

                        auto delta =  opt->at(0).delta();
                        auto cond = ( delta < epsilon_ );
                        if( cond ){
                                std::stringstream sstr;
                                sstr << std::fixed << std::setprecision(10);
                                sstr << "ev_seq_break: delta=" << delta << ", epsilon=" << epsilon_;
                                return Break{sstr.str()};
                        }
                        return Continue{};
                }
        private:
                double epsilon_;
        };



        /*
                This is to detect cyclic solutions

                We have this situation where a partition S = A \union B of the solution set,
                where every sequence of id's in A is constant, and every id in B is non-monotonic

                This excludes the sitation when we have one id which is monotonic, 
         */
        struct state_seq : holdem_binary_solver_any_observer{
                enum{ Lookback = 30 };
                enum{ Debug = true };
                static std::string static_get_name(){ return "state_seq"; }
                state_seq() : holdem_binary_solver_any_observer{static_get_name()}{}

                virtual void imbue(holdem_binary_solver* solver)override{
                        n_ = solver->get_description()->num_players();
                }
                virtual size_t precedence()const override{ return 0; }
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{

                        state_seq_.push_back(to);

                        if( state_seq_.size() < Lookback )
                                return Continue{};


                        auto is_non_increasing = [](auto const& seq){
                                double epsilon =1e-5;
                                bool result = true;
                                for(size_t idx=0;idx+1<seq.size();++idx){
                                        bool cond = ( seq[idx] - epsilon < seq[idx+1]);
                                        if( ! cond )
                                                return false;
                                }
                                return true;
                        };
                        auto is_non_decreasing = [](auto const& seq){
                                double epsilon =1e-5;
                                bool result = true;
                                for(size_t idx=0;idx+1<seq.size();++idx){
                                        bool cond = ( seq[idx] + epsilon > seq[idx+1]);
                                        if( ! cond )
                                                return false;
                                }
                                return true;
                        };
                        auto is_monotonic = [&](auto const& seq){
                                return is_non_increasing(seq) || is_non_decreasing(seq);
                        };
                        auto is_constant = [](auto C, auto const& seq){
                                double epsilon =1e-5;
                                for(auto s : seq){
                                        bool cond = ( std::fabs(s - C ) < epsilon );
                                        if( ! cond )
                                                return false;
                                }
                                return true;
                        };
                        
                        bool result = true;


                        struct Item{
                                size_t idx;
                                size_t cid;
                        };
                        std::vector<Item> constant;
                        std::vector<Item> monotonic;
                        std::vector<Item> non_monoonic;

                        size_t start_idx = state_seq_.size() - Lookback;
                        std::vector<double > seq;

                        // for each degree of state (2 for hu, 6 for three players )
                        for(size_t j=0;j!=n_;++j){
                                // for each class id
                                for(size_t k=0;k!=169;++k){
                                        // for each lag
                                        for(size_t idx=start_idx;idx!=state_seq_.size();++idx){
                                                // want to consider each sequence
                                                auto const& S = state_seq_[idx][j];
                                                seq.push_back(S[k] );
                                        }

                                        if( is_monotonic(seq) ){
                                                if( is_constant(0.0, seq) || is_constant(1.0, seq)){
                                                        constant.push_back(Item{j,k});
                                                } else {
                                                        monotonic.push_back(Item{j,k});
                                                }
                                        } else {
                                                non_monoonic.push_back(Item{j,k});
                                        }
                                        seq.clear();
                                }
                        }

                        bool cyclic_solution = ( monotonic.empty() && non_monoonic.size() );


                        if( cyclic_solution ){
                                std::stringstream sstr;
                                sstr << "state_seq: ";
                                std::vector<holdem_class_vector> aux;
                                aux.resize(n_);
                                for(auto const& p : non_monoonic ){
                                        aux[p.idx].push_back(p.cid);
                                }
                                std::string sep;
                                for(auto const& cv : aux){
                                        sstr << sep << cv;
                                        sep = ",";
                                }

                                if( Debug ){
                                        std::cout << sstr.str() << "\n";
                                }
                                #if 0
                                return Break{sstr.str()};
                                #endif
                                state_seq_.clear();
                                return SmallerFactor{};
                        }
                        return Continue{};
                }
        private:
                size_t n_{0};
                std::vector< state_type > state_seq_;
        };




        struct max_steps_condition : holdem_binary_solver_any_observer{
                explicit max_steps_condition(size_t n):holdem_binary_solver_any_observer{"max_steps_condition"}, n_{n}{}
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const* solver, state_type const& from, state_type const& to)override{
                        if( ++count_ > n_){
                                std::stringstream sstr;
                                sstr << "Done " << n_ << " steps for " << solver->get_description()->string_representation();
                                return Break{sstr.str()};
                        }
                        return Continue{};
                }
        private:
                size_t n_;
                size_t count_{0};
        };
        struct max_steps_error_condition : holdem_binary_solver_any_observer{
                explicit max_steps_error_condition(size_t n):holdem_binary_solver_any_observer{"max_steps_error_condition"}, n_{n}{}
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
                strategy_printer():holdem_binary_solver_any_observer{"strategy_printer"}{}
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
                explicit solver_ledger(std::shared_ptr<holdem_binary_strategy_ledger> ledger)
                        : holdem_binary_solver_any_observer{"solver_ledger"}
                        , ledger_{ledger}
                {}
                virtual holdem_binary_solver_ctrl step(holdem_binary_solver const*, state_type const& from, state_type const& to)override{
                        ledger_->push(to);
                        ledger_->save_();
                        return Continue{};
                }
                virtual void imbue(holdem_binary_solver* solver)override{
                        if( ledger_->size() ){
                                auto state0 = ledger_->back().to_eigen();
                                solver->use_inital_state(std::move(state0));
                        }
                }
        private:
                std::shared_ptr<holdem_binary_strategy_ledger> ledger_;
        };

        /*
                Computing the push/fold table for three players is computation intensive,
                and takes hours to solve for each effective stack, so we need code that
                can handle the restarting, saving of jobs
         */
        struct computation_decl{
                double SB{0.5};
                double BB{1.0};
                size_t N{2};
                std::string Directory;
                std::vector<double> EffectiveStacks;
        };
        struct computation_manager{
                struct work_item{
                        enum{ Debug = true };
                        // args
                        work_item(std::string const& key,
                                  std::string const& ledger_name,
                                  double sb, double bb, size_t n, double eff)
                                :key_{key}
                                ,ledger_name_(ledger_name),
                                sb_(sb),
                                bb_(bb),
                                n_(n),
                                eff_(eff)
                        {
                                switch(n_){
                                        case 2:
                                        {
                                                desc_ = binary_strategy_description::make_hu_description(sb_, bb_, eff_);
                                                break;
                                        }
                                        case 3:
                                        {
                                                desc_ = binary_strategy_description::make_three_player_description(sb_, bb_, eff_);
                                                break;
                                        }
                                        default:
                                        {
                                                throw std::domain_error("unsupported");
                                        }
                                }
                        }
                        double sb()const{ return sb_; }
                        double bb()const{ return bb_; }
                        double eff()const{ return eff_; }
                        binary_strategy_description::strategy_impl_t solution()const{
                                return solution_;
                        }
                        binary_strategy_description const* description()const{ return desc_.get(); }
                        void display()const{
                                std::cout << "sb_ => " << sb_ << "\n"; // __CandyPrint__(cxx-print-scalar,sb_)
                                std::cout << "bb_ => " << bb_ << "\n"; // __CandyPrint__(cxx-print-scalar,bb_)
                                std::cout << "eff_ => " << eff_ << "\n"; // __CandyPrint__(cxx-print-scalar,eff_)

                                for(auto const& s : solution_){
                                        pretty_print_strat(s, 0);
                                        std::cout << "\n";
                                }
                        }

                        void solution_hint(binary_strategy_description::strategy_impl_t const& hint){
                                hint_ = hint;
                        }
                private:
                        friend struct computation_manager;
                        void load(holdem_binary_solution_set* mgr){
                                auto iter = mgr->find(key_);
                                if( iter == mgr->end()){
                                        compute_single();
                                        mgr->add_solution(key_, solution_);
                                        mgr->save_();
                                } else{
                                        solution_ = iter->second.to_eigen();
                                }
                        }
                        void compute_single(){
                                auto ledger = std::make_shared<holdem_binary_strategy_ledger>();
                                if( ledger->try_load_or_default(ledger_name_ ) == false ){
                                        if( hint_ ){
                                                ledger->push(*hint_);
                                        }
                                } else {
                                        std::cout << "loaded ledger of side " << ledger->size() << " (" << ledger_name_ << ")\n";
                                }
                                holdem_binary_solver solver;
                                solver.use_description(desc_);
                                solver.use_strategy(std::make_shared<counter_strategy_aggresive>());
                                //solver.use_strategy(std::make_shared<counter_strategy_elementwise_batch>());
                                if( Debug ){
                                        solver.add_observer(std::make_shared<table_observer>(desc_.get(), true));
                                        solver.add_observer(std::make_shared<strategy_printer>());
                                }
                                solver.add_observer(std::make_shared<solver_ledger>(ledger));
                                solver.add_observer(std::make_shared<lp_inf_stoppage_condition>(lp_epsilon_));
                                solver.add_observer(std::make_shared<max_steps_condition>(max_steps_));
                                solver.add_observer(std::make_shared<state_seq>());

                                auto result = solver.compute();
                                if( result.success()){
                                        for(auto& _ : result.state){
                                                _ = clamp(_);
                                        }
                                }
                                solution_ = result.state;
                        }
                private:
                        // inputs
                        std::string key_;
                        std::string ledger_name_;
                        double sb_;
                        double bb_;
                        size_t n_;
                        double eff_;

                        double lp_epsilon_{0.05};
                        size_t max_steps_{200};


                        // data
                        std::shared_ptr<binary_strategy_description> desc_;
                        //holdem_binary_strategy_ledger ledger;

                        binary_strategy_description::strategy_impl_t solution_;

                        boost::optional<binary_strategy_description::strategy_impl_t> hint_;
                };
                computation_manager(computation_decl const& decl){
                        #if 0
                        if( decl.Directory.size() ){
                                mkdir(decl.Directory.c_str());
                        }
                        #endif
                        for(auto eff : decl.EffectiveStacks ){
                                std::stringstream ledger_name;
                                if( decl.Directory.size() ){
                                        ledger_name << decl.Directory << "/";
                                }
                                ledger_name << decl.N << ":" << decl.SB << ":" << decl.BB << ":" << eff;
                                std::string name = ledger_name.str();
                                std::string key = name; // for now
                                auto item =std::make_shared<work_item>(name,
                                                                       key,
                                                                       decl.SB,
                                                                       decl.BB,
                                                                       decl.N,
                                                                       eff);
                                items_.push_back(item);
                        }
                        mgr_.try_load_or_default(".computation_mgr");
                }
                void compute(){
                        std::vector<std::future<void> > v;
                        for(auto const& ptr : items_){
                                v.push_back(std::async([this,p=ptr](){ p->load(&mgr_); }));
                        }
                }
                void compute_serial(){
                        for(size_t idx=0;idx!=items_.size();++idx){
                                items_[idx]->load(&mgr_);
                                if( idx+1 < items_.size()){
                                        items_[idx+1]->solution_hint( items_[idx]->solution() );
                                }
                        }
                }
                using items_vector_type = std::vector<std::shared_ptr<work_item> >;
                using iterator = boost::indirect_iterator<items_vector_type::const_iterator>;
                iterator begin()const{ return items_.begin(); }
                iterator end()const{ return items_.end(); }
        private:
                items_vector_type items_;
                holdem_binary_solution_set mgr_;
        };

        
        struct SolverCmd : Command{
                struct PrintStrat{
                        void operator()(computation_manager const& mgr)const{
                                using state_type = binary_strategy_description::strategy_impl_t;
                                state_type state;
                                for(auto const& sol : mgr){
                                        auto desc = sol.description();
                                        auto eff        = desc->eff();
                                        auto const& vec = sol.solution();
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
                                for(auto const& s : state){
                                        pretty_print_strat(s, 1);
                                        std::cout << "\n";
                                }
                        }
                };
                struct PrintEv{
                        void operator()(computation_manager const& mgr)const{
                                using namespace Pretty;
                                std::vector<LineItem> lines;
                                lines.push_back(std::vector<std::string>{"Eff", "SB", "BB"});
                                lines.push_back(LineBreak);
                                for(auto const& sol : mgr){
                                        auto desc = sol.description();
                                        auto ev = desc->expected_value(sol.solution());
                                        std::vector<std::string> line;
                                        char buf[18];
                                        std::sprintf(buf, "%.1f", desc->eff());
                                        line.push_back(buf);
                                        for(size_t idx=0;idx!=ev.size();++idx){
                                                line.push_back(boost::lexical_cast<std::string>(ev[idx]));
                                        }
                                        lines.push_back(std::move(line));
                                }
                                RenderTablePretty(std::cout, lines);
                        }
                };
                enum{ Debug = 1};
                explicit
                SolverCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{

                        computation_decl cd;
                        std::string dir = ".SolverCache";
                        cd.Directory = dir;
                        double start_eff = 2.0;

                        double d = 0.1;
                        if( args_.size() && args_[0] == "three"){
                                cd.N = 3;
                                d = 1.0;
                                start_eff = 10.0;
                        }

                        for(double eff=start_eff;eff-1e-5 < 20.0;eff += d ){
                                cd.EffectiveStacks.push_back(eff);
                        }

                        computation_manager mgr(cd);
                        //mgr.compute();
                        mgr.compute_serial();

                        std::vector<std::function<void(computation_manager const&)> > views {
                                PrintStrat{}, PrintEv{} 
                        };
                        for(auto const& view : views){
                                view(mgr);
                        }



                        #if 0
                        std::shared_ptr<binary_strategy_description> desc;
                        std::string ledger_path;
                        if( args_.size() && args_[0] == "three"){
                                desc = binary_strategy_description::make_three_player_description(0.5, 1, 10);
                                ledger_path = ".3_player_ledger.bin";
                        } else{
                                desc = binary_strategy_description::make_hu_description(0.5, 1, 6);
                                ledger_path = ".2_player_ledger.bin";
                        }

                        holdem_binary_solver solver;
                        solver.use_description(desc);
                        solver.use_strategy(std::make_shared<counter_strategy_aggresive>());

                        solver.add_observer(std::make_shared<ev_seq_printer>());
                        //solver.add_observer(std::make_shared<ev_seq_break>(5e-5));
                        solver.add_observer(std::make_shared<table_observer>(desc.get()));
                        //solver.add_observer(std::make_shared<solver_ledger>(ledger_path));
                        solver.add_observer(std::make_shared<strategy_printer>());
                        solver.add_observer(std::make_shared<lp_inf_stoppage_condition>());
                        //solver.add_observer(std::make_shared<ev_diff_stoppage_condition>());

                        auto result = solver.compute();
                        if( auto ptr = boost::get<Break>(&result.stop_condition)){
                                std::cerr << "Break: " << ptr->msg << "\n";
                                for(auto& _ : result.state){
                                        _ = clamp(_);
                                }
                        } else if( auto ptr = boost::get<Error>(&result.stop_condition)){
                                std::cerr << "Error: " << ptr->msg << "\n";
                        } else{
                                std::cerr << "unknown093\n";
                        }
                        #endif

                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<SolverCmd> SolverCmdDecl{"solver"};
        
} // end namespace ps

#endif // PS_CMD_BETTER_SOLVER_H
