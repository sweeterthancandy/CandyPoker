#ifndef PS_EQUITY_CALC_H
#define PS_EQUITY_CALC_H

#include "eval.h"

#include <ostream>
#include <string>

#include <boost/format.hpp>

namespace ps{

        struct equity_player{
                equity_player(long c0, long c1)
                        :card0(c0), card1(c1),
                        wins{0},draw{0},sigma{0},equity{0.0}
                {}

                friend std::ostream& operator<<(std::ostream& ostr, equity_player const& self){
                        auto pct{ 1.0 / self.sigma * 100 };
                        return ostr  << boost::format("{ sigma=%d, wins=%d(%.2f), draw=%d(%.2f), equity=%.2f(%.2f) }")
                                % self.sigma
                                % self.wins % ( self.wins * pct)
                                % self.draw % ( self.draw * pct)
                                % self.equity % ( self.equity * pct);
                }

                long card0;
                long card1;

                size_t wins;
                size_t draw;
                size_t sigma;
                double equity;
                
        };

        namespace detail{
                template<int N, class V, class F, class... Args>
                        std::enable_if_t<N==0> visit_combinations(V v, F f, long upper, Args&&... args){
                                v(std::forward<Args>(args)...);
                        }
                template<int N, class V, class F, class... Args>
                        std::enable_if_t<N!=0> visit_combinations(V v, F f, long upper, Args&&... args){
                                for(long iter{upper+1};iter!=0;){
                                        --iter;
                                        if( ! f(iter) )
                                                continue;
                                        visit_combinations<N-1>(v, f, iter-1,std::forward<Args>(args)..., iter);
                                }
                        }
        }

        struct equity_calc{
                void calc( std::vector<equity_player>& players,
                           std::vector<long> const& board = std::vector<long>{}, 
                           std::vector<long> const& dead = std::vector<long>{}){

                        std::vector<long> known;
                        for( auto const& p : players ){
                                known.emplace_back(p.card0);
                                known.emplace_back(p.card1);
                        }
                        boost::copy( board, std::back_inserter(known));
                        boost::copy( dead, std::back_inserter(known));
                        boost::sort(known);
                        auto filter = [&](long c){ return ! boost::binary_search(known, c); };
                
                        auto do_eval = [&](long a, long b, long c, long d, long e){
                                std::vector<std::pair<std::uint32_t, equity_player* > > ranked;
                                std::vector<equity_player*> winners;
                                for( auto& p : players ){
                                        ++p.sigma;
                                        ranked.emplace_back( std::make_pair(eval_(p.card0, p.card1, a,b,c,d,e), &p));
                                }
                                boost::sort( ranked, [](auto const& l, auto const& r){ return l.first < r.first; });

                                auto iter{ boost::find_if( ranked, [&](auto const& _){ return _.first != ranked.front().first; } ) }; 
                                ranked.resize( iter - ranked.begin());
                                if( ranked.size() == 1 ){
                                        ++ranked.front().second->wins;
                                        ranked.front().second->equity += 1.0;
                                } else{
                                        for( auto const& r : ranked ){
                                                ++r.second->draw;
                                                r.second->equity += 1.0 / ranked.size();
                                        }
                                }
                        };


                        switch(board.size()){
                        case 0:
                                detail::visit_combinations<5>( [&](long a, long b, long c, long d, long e){
                                        do_eval(a,b,c,d,e);
                                }, filter, 51);
                                break;
                        case 1:
                                detail::visit_combinations<4>( [&](long a, long b, long c, long d){
                                        do_eval(board[0], a,b,c,d);
                                }, filter, 51);
                                break;
                        case 2:
                                detail::visit_combinations<3>( [&](long a, long b, long c){
                                        do_eval(board[0], board[1], a,b,c);
                                }, filter, 51);
                                break;
                        case 3:
                                detail::visit_combinations<2>( [&](long a, long b){
                                        do_eval(board[0], board[1], board[2], a,b);
                                }, filter, 51);
                                break;
                        case 4:
                                detail::visit_combinations<1>( [&](long a){
                                        do_eval(board[0], board[1], board[2], board[3], a);
                                }, filter, 51);
                                break;
                        case 5:
                                do_eval( board[0], board[1], board[2], board[3], board[4]);
                                break;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("bad number of board cards"));
                        }
                }
        private:
                eval eval_;
                card_traits traits_;
        };
}

#endif // PS_EQUITY_CALC_H
