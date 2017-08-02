#ifndef PS_CALCULATOR_RESULT_H
#define PS_CALCULATOR_RESULT_H

#include "ps/detail/static_dynamic.h"

namespace ps{


template<class N>
struct basic_result_type{

        using traits = detail::static_dynamic_traits<N>;
        using data_type = typename traits::template array_sq_type< size_t >;

        basic_result_type(basic_result_type const& that)
                :sigma_{ that.sigma_}
                ,n_{ that.n_ }
                ,data_{ that.data_ }
        {
        }
        basic_result_type(basic_result_type&& that)
                :sigma_{ that.sigma_}
                ,n_{ that.n_ }
                ,data_{ that.data_ }
        {
        }

        // only if all convertiable to size_t
        template<class... Args, class = detail::void_t< decltype(static_cast<size_t>(std::declval<Args>()))...> >
        basic_result_type(Args&&... args):
                sigma_{0},
                n_{   detail::get_n<N>(args...)},
                data_{detail::make_sq_array<N, size_t>(args...) }
        {
                #if 0
                PRINT(n_);
                PRINT(data_.size());
                #endif
        }
        size_t const& data_access(size_t i, size_t j)const{
                #if 1
                return data_[i * n_ + j];
                #else
                //PRINT_SEQ((n_)(i)(j)(i * n_ + j)(data_.size()));
                return data_.at(i * n_ + j);
                #endif
        }
        size_t& data_access(size_t i, size_t j){
                return const_cast<size_t&>(
                        reinterpret_cast<basic_result_type const*>(this)
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

        auto n()const{ return n_; }
        
private:
        size_t sigma_;
        size_t n_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        data_type data_;
};


template<class N__>
struct basic_observer_type{

        using result_t = basic_result_type<N__>;

        //basic_observer_type(basic_observer_type const&)=delete;
        
        template<class... Args>
        basic_observer_type(Args&&... args)
                :result_{ std::forward<Args>(args)... }
        {}

        template<class Int, class Vec>
        void operator()(Int a, Int b, Int c, Int d, Int e, Vec const& ranked){
                #if 1
                /*
                        Here I need a quick way to work out the lowest rank,
                        as well as how many are of that rank, and I need to
                        find them. I think this is the quickest
                */
                auto lowest =  ranked[0] ;
                size_t count{1};
                for(size_t i=1;i<ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++count;
                        } else if( ranked[i] < lowest ){
                                lowest = ranked[i]; 
                                count = 1;
                        }
                }
                for(size_t i=0;i!=ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++result_.data_access(i,count-1);
                        }
                }
                ++result_.sigma();
                #else
                std::vector<std::pair<std::uint32_t, size_t> > aux;
                aux.resize(ranked.size());
                for(size_t i{0};i!=aux.size();++i){
                        aux[i] = std::make_pair(ranked[i], i);
                }
                boost::sort( aux, [](auto const& l, auto const& r){ return l.first < r.first; });
                auto winning_rank =  aux.front().first ;
                auto iter{ boost::find_if( aux, [&](auto const& _){ return _.first != winning_rank; } ) }; 
                auto num_winners =  std::distance( aux.begin(), iter) ;

                for( auto j{ aux.begin() }; j!=iter;++j){
                        ++result_.data_access(j->second,num_winners - 1);
                }
                ++result_.sigma();
                #endif
        }  
        template<class View_Type>
        void append(View_Type const& view){
                result_.sigma() += view.sigma();
                for(size_t i=0;i!=result_.n();++i){
                        for(size_t j=0;j!=result_.n();++j){
                                result_.data_access(i, j) += view.player(i).nwin(j);
                        }
                }
        }
        auto const& make()const{ return result_; }
private:
        result_t result_;
};

template<int N_>
using detailed_result_type    = basic_result_type<_int<N_>>;
template<int N_>
using detailed_observer_type = basic_observer_type<_int<N_>>;

using dyn_result_type        = basic_result_type<_dyn>;
using dyn_observer_type      = basic_observer_type<_dyn>;

} // ps

#endif // PS_CALCULATOR_RESULT_H
