#ifndef PS_EVAL_PLAYER_VIEW_H
#define PS_EVAL_PLAYER_VIEW_H

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

} // ps

#endif // PS_EVAL_PLAYER_VIEW_H
