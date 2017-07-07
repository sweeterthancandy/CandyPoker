#ifndef PS_CALCULATOR_H
#define PS_CALCULATOR_H

#include <future>
#include <thread>
#include <numeric>
#include <mutex>
#include <array>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

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
                        #if 0
                        hu_visitor v;
                        ec_.visit_boards(v, players);
                        if( v.win != iter->second.win  ||
                            v.draw != iter->second.draw ||
                            v.lose != iter->second.lose ){
                                PRINT_SEQ((v)(iter->second));
                                asseet(false);
                        }
                        #endif 
                        //hu_result_t aux{ iter->second.permutate(perm) };
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
                        #if 1
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
                        #else
                        return *this;
                        #endif
                }

                size_t sigma;
                std::array<
                        std::array<
                                size_t,
                                N
                        >,
                        N
                > data;

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
                auto make(){ return std::move(result); }
        private:
                result_type result;
        };

};


} // detail
} // ps

#endif // PS_CALCULATOR_H
