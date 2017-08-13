#ifndef EQUITY_EVAL_RESULT_H
#define EQUITY_EVAL_RESULT_H

#include <vector>
#include <ostream>
#include <numeric>

#include "ps/eval/equity_breakdown.h"
#include "ps/support/array_view.h"

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace ps{

struct equity_breakdown_player_matrix : equity_breakdown_player{
        explicit equity_breakdown_player_matrix(size_t n, size_t sigma, support::array_view<size_t> data)
                :n_{n}, sigma_{sigma}, data_{data}
        {}

        double equity()const override{
                double result{0.0};
                for(size_t i=0;i!=n_;++i){
                        result += nwin(i) / (i+1);
                }
                return result / sigma_;
        }
        // nwin(0) -> wins
        // nwin(1) -> draws to split pot 2 ways
        // nwin(2) -> draws to split pot 3 ways
        // ...
        size_t nwin(size_t idx)const override{
                return data_[idx];
        }
        size_t win()const override{  return nwin(0); }
        size_t draw()const override{ return nwin(1); }
        size_t lose()const override{ return sigma_ - std::accumulate( data_.begin(), data_.end(), 0); }
        size_t sigma()const override{ return sigma_; }
        
private:
        size_t n_;
        size_t sigma_;
        support::array_view<size_t> data_;
};


struct equity_breakdown_matrix : equity_breakdown{

        equity_breakdown_matrix():
                n_{0}
        {}
        
        equity_breakdown_matrix(equity_breakdown_matrix const&)=delete;
        equity_breakdown_matrix(equity_breakdown_matrix&&)=delete;
        equity_breakdown_matrix& operator=(equity_breakdown_matrix const&)=delete;
        equity_breakdown_matrix& operator=(equity_breakdown_matrix&&)=delete;

        // only if all convertiable to size_t
        equity_breakdown_matrix(size_t n):
                n_{n},
                data_(n*n)
        {
                for(size_t i=0;i!=n_;++i){
                        players_.emplace_back(n_,
                                     sigma_,
                                     support::array_view<size_t>(&data_[0] + i * n_, n_ ));
                }
        }
        // copy
        equity_breakdown_matrix(equity_breakdown const& that)
                : n_(that.n())
        {
                data_.resize(n_*n_);
                for(size_t i=0;i!=n_;++i){
                        auto const& p = that.player(i);
                        for(size_t j=0;j!=n_;++j){
                                this->data_access(i,j) += p.nwin(j);
                        }
                }
                for(size_t i=0;i!=n_;++i){
                        players_.emplace_back(n_,
                                     sigma_,
                                     support::array_view<size_t>(&data_[0] + i * n_, n_ ));
                }
        }
        size_t const& data_access(size_t i, size_t j)const{
                return data_.at(i * n_ + j);
                //return data_[i * n_ + j];
                //PRINT_SEQ((n_)(i)(j)(i * n_ + j)(data_.size()));
        }
        size_t& data_access(size_t i, size_t j){
                return const_cast<size_t&>(
                        reinterpret_cast<equity_breakdown_matrix const*>(this)
                                ->data_access(i,j)
                );
        }
        size_t const* data()const{ return reinterpret_cast<size_t const*>(&data_.front()); }


        size_t sigma()const override{ return sigma_; }
        size_t& sigma(){ return sigma_; }

        equity_breakdown_player const& player(size_t idx)const override{
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
                for(size_t i=0;i!=n_;++i){
                        players_.emplace_back(n_,
                                              sigma_,
                                              support::array_view<size_t>(&data_[0] + i * n_, n_ ));
                }

        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        
private:
        size_t sigma_ = 0;
        size_t n_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::vector<size_t> data_;

        // need to allocate these
        std::vector< equity_breakdown_player_matrix> players_;
};
        
struct equity_breakdown_matrix_aggregator : equity_breakdown_matrix{

        explicit equity_breakdown_matrix_aggregator(size_t n)
                : equity_breakdown_matrix{n}
        {}

        void append(equity_breakdown const& breakdown){
                assert( breakdown.n() == n()     && "precondition failed");
                sigma() += breakdown.sigma();
                for(size_t i=0;i!=n();++i){
                        for(size_t j=0;j!=n();++j){
                                data_access(i, j) += breakdown.player(i).nwin(j);
                        }
                }
        }
        void append_matrix(equity_breakdown const& breakdown, std::vector<int> const& mat){
                assert( breakdown.n() == n()     && "precondition failed");
                assert( mat.size()    == n()*n() && "precondition failed");
                sigma() += breakdown.sigma();
                for( int i =0; i!= n();++i){
                        auto& p = breakdown.player(i);
                        for( int j =0;j!=n();++j){
                                for( int k =0;k!=n();++k){
                                        data_access(j,k) += p.nwin(k) * mat[ i * n() + j];
                                }
                        }
                }
        }
};


} // ps

#endif // EQUITY_EVAL_RESULT_H
