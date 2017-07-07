#ifndef PS_CALCULATOR_H
#define PS_CALCULATOR_H

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




template<class Result_Decl, size_t N>
struct basic_calculator_N{
        using result_type   = typename Result_Decl::result_type;
        using observer_type = typename Result_Decl::observer_type;
        using player_vec_t   = std::array<ps::holdem_id, N>;

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

        result_type calculate( std::array<ps::holdem_id, N> const& players){
                std::vector<ps::holdem_id> aux{ players.begin(), players.end() };
                auto p{ permutate_for_the_better(aux) };
                auto const& perm{std::get<0>(p)};
                std::array<ps::holdem_id, N> perm_players;
                for(size_t i=0;i!=N;++i){
                        perm_players[i] = std::get<1>(p)[i];
                }

                auto iter = cache_.find(perm_players);
                if( iter != cache_.end() ){
                        return iter->second.permutate(perm);
                }

                observer_type observer;
                ec_->visit_boards(observer, perm_players);
                cache_.insert(std::make_pair(perm_players, observer.make()));
                return calculate(players);
        }
private:
        std::map< std::array< ps::holdem_id, N>, result_type> cache_;
        equity_calc_detail* ec_;
};



template<class Result_Decl, size_t N, class Impl>
struct basic_class_calculator_N{
        using result_type   = typename Result_Decl::result_type;
        using observer_type = typename Result_Decl::observer_type;
        using player_vec_t  = std::array<ps::holdem_id, N>;
        using impl_t        = Impl;
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
                                int dum[]= {0, ( std::cout << Ints << ":" << args << ":" <<
                                                 hand_sets[Ints]->operator[](args) << " vs " , 0)...};
                                std::cout << "\n";
                                PRINT(result);

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
        result_type calculate( std::array<ps::holdem_class_id, N> const& players){
                auto iter{ cache_.find( players) };
                if( iter != cache_.end() ){
                        return iter->second;
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

                cache_.insert(std::make_pair(players, detail_.observer.make()));
                return calculate(players);
        }
private:
        std::map< std::array< ps::holdem_class_id, N>, result_type> cache_;
        impl_t* impl_;
};




template<size_t N>
struct basic_detailed_calculation_decl{

        struct result_type{
                result_type(result_type const&)=default;
                result_type():
                        sigma{0}
                {
                        std::memset( data.begin(), 0, sizeof(data));
                }

                template<class Con>
                result_type permutate(Con const& con)const{
                        result_type ret;
                        ret.sigma = sigma;
                        size_t q{0};
                        for( auto idx : con ){
                                for(size_t i=0;i!=N;++i){
                                        ret.data[idx][i] = this->data[q][i];
                                }
                                ++q;
                        }
                        return std::move(ret);
                }

                struct view_t{
                        explicit view_t(result_type const* self, size_t idx)
                                :this_(self),idx_{idx}
                        {}
                        double equity()const{
                                double result{0.0};
                                for(size_t i=0;i!=N;++i){
                                        result += this_->data[idx_][i] / (i+1);
                                }
                                return result / this_->sigma;
                        }
                        size_t sigma()const{
                                return this_->sigma;
                        }
                private:
                        size_t idx_;
                        result_type const* this_;
                };
                auto player(size_t idx)const{ return view_t{this, idx}; }

                size_t sigma;
                std::array<
                        std::array<
                                size_t,
                                N
                        >,
                        N
                > data;
                template<class Archive>
                void serialize(Archive& ar, unsigned int){
                        ar & sigma;
                        ar & data;
                }

                friend std::ostream& operator<<(std::ostream& ostr, result_type const& self){
                        std::vector<std::vector<std::string> > line_buffer;
                        std::array<size_t, N> widths;
                        widths.fill(0);
                        std::cout << self.sigma << "\n";
                        for(size_t i=0;i!=N;++i){
                                line_buffer.emplace_back();
                                for(size_t j=0;j!=N;++j){
                                        line_buffer.back().emplace_back(
                                                boost::lexical_cast<std::string>(
                                                        self.data[i][j]));
                                        widths[j] = std::max(widths[j], line_buffer.back().back().size());
                                }
                        }
                        for(size_t i=0;i!=N;++i){
                                for(size_t j=0;j!=N;++j){
                                        auto const& tok(line_buffer[i][j]);
                                        size_t padding{widths[j]-tok.size()};
                                        size_t left_pad{padding/2};
                                        size_t right_pad{padding - left_pad};
                                        if( j != 0 ){
                                                ostr << " | ";
                                        }
                                        if( left_pad )
                                                ostr << std::string(left_pad,' ');
                                        ostr << tok;
                                        if( right_pad )
                                                ostr << std::string(right_pad,' ');

                                }
                                ostr << "\n";
                        }
                        return ostr;
                }
        };

        struct observer_type{
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
                                        ++result.data[i][count-1];
                                }
                        }
                        ++result.sigma;
                }  
                void append(result_type const& param){
                        result.sigma += param.sigma;
                        for(size_t i=0;i!=N;++i){
                                for(size_t j=0;j!=N;++j){
                                        result.data[i][j] += param.data[i][j];
                                }
                        }
                }
                auto make(){ return result; }
        private:
                result_type result;
        };

};


} // detail
} // ps

#endif // PS_CALCULATOR_H
