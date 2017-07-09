#ifndef PS_CALCULATOR_DETAIL_H
#define PS_CALCULATOR_DETAIL_H

#include <future>
#include <utility>
#include <thread>
#include <numeric>
#include <mutex>
#include <array>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>

#include "ps/cards_fwd.h"
#include "ps/equity_calc_detail.h"
#include "ps/algorithm.h"

#include "ps/detail/array_view.h"

#include "ps/calculator_view.h"


/*
        For equity calculations, creating pre-computation
        databases is important, which leads to a situation of
        a database of every hand vs hand situation, where
        for every unique hand vs hand we have a 3-tuple
        (win,draw,lose). For hand vs hand we can trivally 
        take into account the case where
                        a vs b -> (win,draw,lose)
                =>      b vs a -> (lose,draw,win).
        However, for n vs n players, each unique situation
        will create a n x n matrix of results, and the size 
        of the cache created might be too large.
                Also, most of our calculation is only
        concerned with the equity% of the hand, which leads
        to a situation where we want different information
        from the calculator.
                The idea of creating an N parameter for
        each structure, is that if I was to cache every
        2-9 player calculations, without thinking too much
        I would of thought it would be more effiecent to
        split up each n-player calculation by it'self




        abstractions


*/

namespace ps{
namespace detail{

/*
        Win Draw Draw2 Draw3 Draw4 Draw5 Draw6 Draw7 Draw8 Draw9
 */






template<size_t N>
struct detailed_result_type{
        detailed_result_type(detailed_result_type const&)=default;
        detailed_result_type():
                sigma_{0}
        {
                std::memset( data_.begin(), 0, sizeof(data_));
        }
        auto& data_access(size_t i, size_t j){
                return data_[i * N + j];
        }
        auto data_access(size_t i, size_t j)const{
                return data_[i * N + j];
        }
        size_t const* data()const{ return reinterpret_cast<size_t const*>(data_.begin()); }

        #if 0
        template<class Con>
        result_type permutate(Con const& con)const{
                result_type ret;
                ret.sigma_ = sigma_;
                size_t q{0};
                for( auto idx : con ){
                        for(size_t i=0;i!=N;++i){
                                ret.data_[idx][i] = this->data_[q][i];
                        }
                        ++q;
                }
                return std::move(ret);
        }
        #endif



        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & sigma_;
                ar & data_;
        }

        auto sigma()const{ return sigma_; }
        auto& sigma(){ return sigma_; }
        
private:
        size_t sigma_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::array<
                        size_t,
                N * N
        > data_;
};


template<size_t N>
struct detailed_observer_type{
        template<class Int, class Vec>
        void operator()(Int a, Int b, Int c, Int d, Int e, Vec const& ranked){
                /*
                        Here I need a quick way to work out the lowest rank,
                        as well as how many are of that rank, and I need to
                        find them. I think this is the quickest
                */
                auto lowest{ ranked[0] };
                size_t count{1};
                for(size_t i=1;i<ranked.size();++i){
                        if( ranked[i] == lowest )
                                ++count;
                        else if( ranked[i] < lowest ){
                                lowest = ranked[i]; 
                                count = 1;
                        }
                }
                for(size_t i=0;i!=ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++result.data_access(i,count-1);
                        }
                }
                ++result.sigma();
        }  
        template<class View_Type>
        void append(View_Type const& view){
                result.sigma() += view.sigma();
                for(size_t i=0;i!=N;++i){
                        for(size_t j=0;j!=N;++j){
                                result.data_access(i, j) += view.player(i).nwin(j);
                        }
                }
        }
        auto make(){ return result; }
private:
        detailed_result_type<N> result;
};


template<size_t N>
struct basic_calculator_N{
        using view_type     = detailed_view_type;
        using result_type   = detailed_result_type<N>;
        using observer_type = detailed_observer_type<N>;
        using player_vec_t  = std::array<ps::holdem_id, N>;
        using this_t        = basic_calculator_N;

        explicit basic_calculator_N(equity_calc_detail* ec):ec_{ec}{}

        bool load(std::string const& name){
                std::ifstream is(name);
                if( ! is.is_open() )
                        return false;
                boost::archive::text_iarchive ia(is);
                ia >> *this;
                return true;
        }
        bool save(std::string const& name)const{
                std::ofstream of(name);
                boost::archive::text_oarchive oa(of);
                oa << *this;
                return true;
        }
        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & cache_;
        }

        // TODO remove duplication
        view_type calculate( detail::array_view<ps::holdem_id> const& players){
                assert( players.size() == N && "precondition failed");
                std::vector<ps::holdem_id> aux{ players.begin(), players.end() };
                auto p{ permutate_for_the_better(aux) };
                auto const& perm{std::get<0>(p)};
                std::array<ps::holdem_id, N> perm_players;
                for(size_t i=0;i!=N;++i){
                        perm_players[i] = std::get<1>(p)[i];
                }

                auto iter = cache_.find(perm_players);
                if( iter != cache_.end() ){
                        return view_type{
                                N,
                                iter->second.sigma(),
                                detail::array_view<size_t>{ iter->second.data(), N},
                                std::move(perm)
                        };
                }

                observer_type observer;
                ec_->visit_boards(observer, perm_players);
                cache_.insert(std::make_pair(perm_players, observer.make()));
                return calculate(players);
        }
        void append(this_t const& that){
                for( auto const& p : that.cache_ )
                        this->cache_.insert(p);
        }
private:
        std::map< player_vec_t, result_type> cache_;
        equity_calc_detail* ec_;
};



template<size_t N>
struct basic_class_calculator_N{
        using view_type     = detailed_view_type;
        using result_type   = detailed_result_type<N>;
        using observer_type = detailed_observer_type<N>;
        using player_vec_t  = std::array<ps::holdem_id, N>;
        using impl_t        = basic_calculator_N<N>;
        using this_t        = basic_class_calculator_N;

        explicit basic_class_calculator_N(impl_t* impl):impl_{impl}{}

        bool load(std::string const& name){
                std::ifstream is(name);
                if( ! is.is_open() )
                        return false;
                boost::archive::text_iarchive ia(is);
                ia >> *this;
                return true;
        }
        bool save(std::string const& name)const{
                std::ofstream of(name);
                boost::archive::text_oarchive oa(of);
                oa << *this;
                return true;
        }
        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & cache_;
        }
        void append(this_t const& that){
                for( auto const& p : that.cache_ )
                        this->cache_.insert(p);
        }

private:

        // Can't have this local to calculate
        struct local_detail{
                template<size_t... Ints, class... size_t_>
                void impl(std::index_sequence<Ints...> , size_t_... args){

                        if( disjoint(
                                holdem_hand_decl::get( hand_sets[Ints]->operator[](args).id())...
                                    ) )
                        {
                                auto result{
                                        this_->impl_->calculate(std::array<ps::holdem_id, N>{
                                                                holdem_hand_decl::get( hand_sets[Ints]->operator[](args).id())...
                                                                 })
                                           };
                                #if 0
                                int dum[]= {0, ( std::cout << Ints << ":" << args << ":" <<
                                                 hand_sets[Ints]->operator[](args) << " vs " , 0)...};
                                std::cout << "\n";
                                PRINT(result);
                                #endif

                                observer.append(result);
                        }
                }
                template<class... size_t_, class seq = std::make_index_sequence<sizeof...(size_t_)> >
                void operator()(size_t_... args){
                        impl( seq{}, args... );
                }
                this_t* this_;
                observer_type observer;
                std::array< std::vector<holdem_hand_decl> const*, N> hand_sets;
        };
public:
        view_type calculate( detail::array_view<ps::holdem_class_id> const& players){
                assert( players.size() == N && "precondition failed");
                std::array< ps::holdem_class_id, N> proto;
                for(size_t i=0;i!=N;++i)
                        proto[i] = players[i];
                auto iter{ cache_.find( proto) };
                if( iter != cache_.end() ){
                        std::vector<int> perm;
                        for(size_t i=0;i!=N;++i)
                                perm.emplace_back(i);
                        return view_type{
                                N,
                                iter->second.sigma(),
                                detail::array_view<size_t>{ iter->second.data(), N},
                                std::move(perm)
                        };
                }
                std::array<size_t, N> size_vec;
                local_detail detail_;
                for(size_t i=0;i!=N;++i){
                        detail_.hand_sets[i] = &holdem_class_decl::get(players[i]).get_hand_set();
                        size_vec[i] = detail_.hand_sets[i]->size() -1;
                }

                detail_.this_ = this;

                // TODO test disjoint at each start
                // TODO make this interface better
                detail::visit_exclusive_combinations<N>( std::ref(detail_), detail::true_, size_vec);

                cache_.insert(std::make_pair(proto, detail_.observer.make()));
                return calculate(players);
        }
private:
        std::map< std::array< ps::holdem_class_id, N>, result_type> cache_;
        impl_t* impl_;
};
                


} // detail
} // ps

#endif // PS_CALCULATOR_DETAIL_H
