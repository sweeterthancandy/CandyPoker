#ifndef PS_EVAL_AGGREGATOR_H
#define PS_EVAL_AGGREGATOR_H

namespace ps{
        struct aggregator{
                using view_t = detailed_view_type;

                void append(view_t const& view){
                        if( n_ == 0 ){
                                n_ = view.n();
                                data_.resize( n_ * n_ );
                        }
                        sigma_ += view.sigma();
                        for(size_t i=0;i!=n_;++i){
                                for(size_t j=0;j!=n_;++j){
                                        data_access(i, j) += view.player(i).nwin(j);
                                }
                        }
                }
                view_t make_view(){
                        std::vector<int> perm;
                        for(size_t i=0;i!=n_;++i)
                                perm.emplace_back(i);
                        return view_t{
                                n_,
                                sigma_,
                                support::array_view<size_t>{ data_},
                                std::move(perm)
                        };
                }
        private:
                size_t& data_access(size_t i,size_t j){
                        return data_[i * n_ + j];
                }
                size_t n_=0;
                size_t sigma_ =0;
                std::vector< size_t > data_;
        };
} // ps
#endif // PS_EVAL_AGGREGATOR_H
