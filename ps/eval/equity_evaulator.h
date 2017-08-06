#ifndef PS_EVAL_EVALUATOR_H
#define PS_EVAL_EVALUATOR_H

#include <vector>
#include <ostream>
#include <numeric>

#include "ps/support/array_view.h"
#include "ps/support/singleton_factory.h"

namespace ps{
        
struct player_view_t{
        explicit player_view_t(size_t n, size_t sigma, support::array_view<size_t> data)
                :n_{n}, sigma_{sigma}, data_{data}
        {}
        double equity()const{
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
        size_t nwin(size_t idx)const{
                return data_[idx];
        }
        size_t win()const{  return nwin(0); }
        size_t draw()const{ return nwin(1); }
        size_t lose()const{ return sigma_ - std::accumulate( data_.begin(), data_.end(), 0); }
        size_t sigma()const{ return sigma_; }

private:
        size_t n_;
        size_t sigma_;
        support::array_view<size_t> data_;
};

struct equity_eval_result{

        // only if all convertiable to size_t
        equity_eval_result(size_t n):
                n_{n},
                data_(n*n)
        {
        }
        size_t const& data_access(size_t i, size_t j)const{
                return data_.at(i * n_ + j);
                //return data_[i * n_ + j];
                //PRINT_SEQ((n_)(i)(j)(i * n_ + j)(data_.size()));
        }
        size_t& data_access(size_t i, size_t j){
                return const_cast<size_t&>(
                        reinterpret_cast<equity_eval_result const*>(this)
                                ->data_access(i,j)
                );
        }
        size_t const* data()const{ return reinterpret_cast<size_t const*>(&data_.front()); }

        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & sigma_;
                ar & data_;
        }

        auto sigma()const{ return sigma_; }
        auto& sigma(){ return sigma_; }

        auto player(size_t idx)const{
                return player_view_t(n_,
                                     sigma_,
                                     support::array_view<size_t>(&data_[0] + n_ , n_ ));
        }

        auto n()const{ return n_; }
        
private:
        size_t sigma_ = 0;
        size_t n_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::vector<size_t> data_;
};



struct equity_evaluator{
        virtual ~equity_evaluator()=default;

        virtual equity_eval_result evaluate(std::vector<holdem_id> const& players)const=0;
};


using equity_evaluator_factory = support::singleton_factory<equity_evaluator>;

} // ps

#endif // #ifndef PS_EVAL_EVALUATOR_H
