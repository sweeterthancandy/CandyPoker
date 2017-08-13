#include "ps/eval/equity_breakdown_matrix.h"

namespace ps{
        equity_breakdown_player_matrix::equity_breakdown_player_matrix(size_t n, size_t sigma, support::array_view<size_t> data)
                :n_{n}, sigma_{sigma}, data_{data}
        {}

        double equity_breakdown_player_matrix::equity()const {
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
        size_t equity_breakdown_player_matrix::nwin(size_t idx)const {
                return data_[idx];
        }
        size_t equity_breakdown_player_matrix::win()const {  return nwin(0); }
        size_t equity_breakdown_player_matrix::draw()const { return nwin(1); }
        size_t equity_breakdown_player_matrix::lose()const { return sigma_ - std::accumulate( data_.begin(), data_.end(), 0); }
        size_t equity_breakdown_player_matrix::sigma()const { return sigma_; }
        
        equity_breakdown_matrix::equity_breakdown_matrix():
                n_{0}
        {}
        
        equity_breakdown_matrix::equity_breakdown_matrix(equity_breakdown_matrix const& that)
                :n_{that.n_},
                data_{that.data_}
        {
                for(size_t i=0;i!=n_;++i){
                        players_.emplace_back(n_,
                                     sigma_,
                                     support::array_view<size_t>(&data_[0] + i * n_, n_ ));
                }
        }
        // only if all convertiable to size_t
        equity_breakdown_matrix::equity_breakdown_matrix(size_t n):
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
        equity_breakdown_matrix::equity_breakdown_matrix(equity_breakdown const& that)
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
        equity_breakdown_matrix::equity_breakdown_matrix(equity_breakdown const& that, std::vector<int> const& perm)
                : n_(that.n())
        {
                data_.resize(n_*n_);
                for(size_t i=0;i!=n_;++i){
                        auto const& p = that.player(perm[i]);
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
        size_t const& equity_breakdown_matrix::data_access(size_t i, size_t j)const{
                return data_.at(i * n_ + j);
                //return data_[i * n_ + j];
                //PRINT_SEQ((n_)(i)(j)(i * n_ + j)(data_.size()));
        }
        size_t& equity_breakdown_matrix::data_access(size_t i, size_t j){
                return const_cast<size_t&>(
                        reinterpret_cast<equity_breakdown_matrix const*>(this)
                                ->data_access(i,j)
                );
        }
        size_t const* equity_breakdown_matrix::data()const{ return reinterpret_cast<size_t const*>(&data_.front()); }


        size_t equity_breakdown_matrix::sigma()const { return sigma_; }
        size_t& equity_breakdown_matrix::sigma(){ return sigma_; }

        equity_breakdown_player const& equity_breakdown_matrix::player(size_t idx)const {
                return players_[idx];
        }

        size_t equity_breakdown_matrix::n()const { return n_; }
        
        
        
        
        equity_breakdown_matrix_aggregator::equity_breakdown_matrix_aggregator(size_t n)
                : equity_breakdown_matrix{n}
        {}

        void equity_breakdown_matrix_aggregator::append(equity_breakdown const& breakdown){
                assert( breakdown.n() == n()     && "precondition failed");
                sigma() += breakdown.sigma();
                for(size_t i=0;i!=n();++i){
                        for(size_t j=0;j!=n();++j){
                                data_access(i, j) += breakdown.player(i).nwin(j);
                        }
                }
        }
        void equity_breakdown_matrix_aggregator::append_perm(equity_breakdown const& breakdown, std::vector<int> const& perm){
                assert( breakdown.n() == n()     && "precondition failed");
                assert( mat.size()    == n()     && "precondition failed");

                for( int i =0;i!=n();++i){
                        for( int j =0;j!=n();++j){
                                data_access(i,j) += player(perm[i]).nwin(j);
                        }
                }
        }
        void equity_breakdown_matrix_aggregator::append_matrix(equity_breakdown const& breakdown, std::vector<int> const& mat){
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
} // ps
