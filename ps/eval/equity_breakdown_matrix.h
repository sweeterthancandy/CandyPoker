#ifndef EQUITY_EVAL_RESULT_H
#define EQUITY_EVAL_RESULT_H

#include <vector>
#include <ostream>
#include <numeric>

#include "ps/eval/equity_breakdown.h"
#include "ps/support/array_view.h"
#include "ps/detail/print.h"

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace ps{



template<class Primitive_Type>
struct basic_equity_breakdown_matrix : basic_equity_breakdown<Primitive_Type>{

        using prim_t   = typename basic_equity_breakdown<Primitive_Type>::prim_t;
        using player_t = typename basic_equity_breakdown<Primitive_Type>::player_t;

        struct equity_breakdown_player_matrix : player_t{
                explicit equity_breakdown_player_matrix(size_t n, prim_t* sigma, support::array_view<prim_t> data)
                        :n_{n}, sigma_{sigma}, data_{data}
                {}

                double equity()const override{
                        double result{0.0};
                        for(size_t i=0;i!=n_;++i){
                                result += nwin(i) / (i+1);
                        }
                        result /= sigma();
                        //PRINT(result);
                        return result;
                }
                // nwin(0) -> wins
                // nwin(1) -> draws to split pot 2 ways
                // nwin(2) -> draws to split pot 3 ways
                // ...
                prim_t nwin(size_t idx)const override{
                        return data_[idx];
                }
                prim_t win()const override{  return nwin(0); }
                prim_t draw()const override{ return nwin(1); }
                prim_t lose()const override{ return sigma() - std::accumulate( data_.begin(), data_.end(), 0); }
                prim_t sigma()const override{ return *sigma_; }
                
        private:
                size_t n_;
                prim_t* sigma_;
                support::array_view<prim_t> data_;
        };

        basic_equity_breakdown_matrix():
                n_{0}
        {}
        
        basic_equity_breakdown_matrix(basic_equity_breakdown_matrix const& that)
                :n_{that.n_},
                sigma_{that.sigma()},
                data_{that.data_}
        {
                cache_players_();
        }
        basic_equity_breakdown_matrix(basic_equity_breakdown_matrix&&)=delete;
        basic_equity_breakdown_matrix& operator=(basic_equity_breakdown_matrix const&)=delete;
        basic_equity_breakdown_matrix& operator=(basic_equity_breakdown_matrix&&)=delete;

        // only if all convertiable to size_t
        basic_equity_breakdown_matrix(size_t n):
                n_{n},
                data_(n*n)
        {
                cache_players_();
        }
        // copy
        basic_equity_breakdown_matrix(basic_equity_breakdown<Primitive_Type> const& that)
                : n_{that.n()}
                , sigma_{that.sigma()}
        {
                data_.resize(n_*n_);

                for(size_t i=0;i!=n_;++i){
                        auto const& p = that.player(i);
                        for(size_t j=0;j!=n_;++j){
                                this->data_access(i,j) += p.nwin(j);
                        }
                }
                cache_players_();
        }
        basic_equity_breakdown_matrix(basic_equity_breakdown<Primitive_Type> const& that, std::vector<int> const& perm)
                : n_(that.n())
                , sigma_{ that.sigma() }
        {
                data_.resize(n_*n_);
                for(size_t i=0;i!=n_;++i){
                        auto const& p = that.player(perm[i]);
                        for(size_t j=0;j!=n_;++j){
                                this->data_access(i,j) += p.nwin(j);
                        }
                }
                cache_players_();
        }
        prim_t const& data_access(size_t i, size_t j)const{
                return data_.at(i * n_ + j);
                //return data_[i * n_ + j];
                //PRINT_SEQ((n_)(i)(j)(i * n_ + j)(data_.size()));
        }
        prim_t& data_access(size_t i, size_t j){
                return const_cast<prim_t&>(
                        reinterpret_cast<basic_equity_breakdown_matrix const*>(this)
                                ->data_access(i,j)
                );
        }
        prim_t const* data()const{ return reinterpret_cast<prim_t const*>(&data_.front()); }


        prim_t sigma()const override{ return sigma_; }
        prim_t& sigma(){ return sigma_; }

        equity_breakdown_player_matrix const& player(size_t idx)const override{
                return players_[idx];
        }

        size_t n()const override{ return n_; }
        
        template<class Archive>
        void save(Archive &ar, const unsigned int version)const
        {
                ar & sigma_;
                ar & n_;
                ar & data_;
        }
        template<class Archive>
        void load(Archive &ar, const unsigned int version)
        {
                ar & sigma_;
                ar & n_;
                ar & data_;
                players_.clear();
                cache_players_();

        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()


        
private:
        void cache_players_(){
                for(size_t i=0;i!=n_;++i){
                        players_.emplace_back(n_,
                                     &sigma_,
                                     support::array_view<prim_t>(&data_[0] + i * n_, n_ ));
                }
        }

        size_t n_;
        prim_t sigma_ = 0;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::vector<prim_t> data_;

        // need to allocate these
        std::vector< equity_breakdown_player_matrix> players_;
};

        
template<class Primitive_Type>
struct basic_equity_breakdown_matrix_aggregator : basic_equity_breakdown_matrix<Primitive_Type>{

        explicit basic_equity_breakdown_matrix_aggregator(size_t n)
                : basic_equity_breakdown_matrix<Primitive_Type>{n}
        {}

        using basic_equity_breakdown_matrix<Primitive_Type>::sigma;
        using basic_equity_breakdown_matrix<Primitive_Type>::data_access;
        using basic_equity_breakdown_matrix<Primitive_Type>::n;


        void append(basic_equity_breakdown<Primitive_Type> const& breakdown){
                assert( breakdown.n() == n()     && "precondition failed");
                sigma() += breakdown.sigma();
                for(size_t i=0;i!=n();++i){
                        for(size_t j=0;j!=n();++j){
                                data_access(i, j) += breakdown.player(i).nwin(j);
                        }
                }
        }
        template<class OtherPrimitive_Type, class T>
        void append_scalar(basic_equity_breakdown<OtherPrimitive_Type> const& breakdown, T scalar){
                assert( breakdown.n() == n()     && "precondition failed");
                sigma() += breakdown.sigma() * scalar;
                for(size_t i=0;i!=n();++i){
                        for(size_t j=0;j!=n();++j){
                                data_access(i, j) += breakdown.player(i).nwin(j) * scalar;
                        }
                }
        }
        template<class T>
        void append_perm(basic_equity_breakdown<Primitive_Type> const& breakdown, std::vector<T> const& perm){
                assert( breakdown.n() == n()     && "precondition failed");
                assert( perm.size()    == n()    && "precondition failed");
                sigma() += breakdown.sigma();

                for( size_t i =0;i!=n();++i){
                        for( size_t j =0;j!=n();++j){
                                data_access(i,j) += breakdown.player(perm[i]).nwin(j);
                        }
                }
        }
        template<class T>
        void append_matrix(basic_equity_breakdown<Primitive_Type> const& breakdown, std::vector<T> const& mat){
                assert( breakdown.n() == n()     && "precondition failed");
                assert( mat.size()    == n()*n() && "precondition failed");
                /*
                        TODO

                        proper way to fix this is a lose column

                        If we have mat = P0 + P1 + P2  + ... + Pn, for a sequence of n
                        permutation matrixies, then this will work
                 */
                sigma() += ( breakdown.sigma() * std::accumulate(mat.begin(), mat.end(), 0) / n() );
                for( size_t i =0; i!= n();++i){
                        auto& p = breakdown.player(i);
                        for( size_t j =0;j!=n();++j){
                                for( size_t k =0;k!=n();++k){
                                        data_access(j,k) += p.nwin(k) * mat[ i * n() + j];
                                }
                        }
                }
        }
};

using equity_breakdown_matrix             = basic_equity_breakdown_matrix<size_t>;
using equity_breakdown_matrix_aggregator  = basic_equity_breakdown_matrix_aggregator<size_t>;
using fequity_breakdown_matrix            = basic_equity_breakdown_matrix<double>;
using fequity_breakdown_matrix_aggregator = basic_equity_breakdown_matrix_aggregator<double>;




} // ps

#endif // EQUITY_EVAL_RESULT_H
