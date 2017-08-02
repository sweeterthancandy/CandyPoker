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

#include "ps/support/array_view.h"

#include "ps/calculator_view.h"
#include "ps/calculator_result.h"


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
        view_type calculate( support::array_view<ps::holdem_id> const& players){
                assert( players.size() == N && "precondition failed");
                auto p =  permutate_for_the_better<N>(players) ;
                auto const& perm = std::get<0>(p);
                auto const& perm_players = std::get<1>(p);

                auto iter = cache_.find(perm_players);
                if( iter != cache_.end() ){
                        return view_type{
                                N,
                                iter->second.sigma(),
                                support::array_view<size_t>{ iter->second.data(), N},
                                std::move(std::vector<int>{perm.begin(), perm.end()})
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
                                auto result = this_->impl_->calculate(std::array<ps::holdem_id, N>{
                                        holdem_hand_decl::get( hand_sets[Ints]->operator[](args).id())...
                                });

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
        view_type calculate( support::array_view<ps::holdem_class_id> const& players){
                assert( players.size() == N && "precondition failed");
                std::array< ps::holdem_class_id, N> proto;
                for(size_t i=0;i!=N;++i)
                        proto[i] = players[i];
                auto iter =  cache_.find( proto) ;
                if( iter != cache_.end() ){
                        return view_type{
                                N,
                                iter->second.sigma(),
                                support::array_view<size_t>{ iter->second.data(), N}
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
