#ifndef PS_EQUITY_CALC_H
#define PS_EQUITY_CALC_H


#include <ostream>
#include <string>

#include <boost/format.hpp>

#include "ps/core/eval.h"
#include "ps/detail/visit_combinations.h"


namespace ps{


        // AkAh vs 2c2c      atomic
        // AA vs T9s         
        //
        // AcAd       Tc9c
        // AcAh       Td9d
        // AcAs   VS  Th9h
        // AhAd       Ts9s
        // AhAs
        // AsAs
        //
        // Ako,KK+ vs 89s+,TT+,AQo+

        struct equity_player{
                equity_player(long c0 = 0, long c1 = 0)
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
        
        struct equity_simulation{
                std::vector<long> board;
                std::vector<equity_player> players;
        };


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

        struct hu_equity_calc{
                hu_equity_calc():
                        cache_(52*52*52*52)
                {
                        equity_calc ec;
                        auto yeild = [&](long h0, long h1, long v0, long v1){
                                std::vector<equity_player> players;
                                players.emplace_back(h0,h1);
                                players.emplace_back(v0,v1);
                                ec.calc(players);
                                assert( cache_[map_(h0,h1,v0,v1)] && "remapping");
                                std::cout 
                                        << traits_.to_string(h0)
                                        << traits_.to_string(h1)
                                        << " VS "
                                        << traits_.to_string(v0)
                                        << traits_.to_string(v1)
                                        << " "
                                        << players.front() << "\n";
                                cache_[map_(h0,h1,v0,v1)] = players.front();
                                cache_[map_(v0,v1,h0,h1)] = players.back();
                        };
                        detail::visit_combinations<4>( [&](long a, long b, long c, long d){
                                yeild(a,b,c,d);
                                yeild(a,c,b,d);
                                yeild(a,d,b,c);
                        }, [](auto){return true; }, 51);
                }
                equity_player const& operator()(long h0, long h1, long v0, long v1)const{
                        if( h1 > h0)
                                std::swap(h0,h1);
                        if( v1 > v0)
                                std::swap(v0,v1);
                        assert( cache_[map_(h0,h1,v0,v1)] != 0 && "no mapping");
                        return cache_[ map_(h0,h1,v0,v1)];
                }
                equity_player const& operator()(std::string const& hero, std::string const& villian)const{
                        return (*this)(traits_.make(hero.substr(0,2)),
                                       traits_.make(hero.substr(2,2)),
                                       traits_.make(villian.substr(0,2)),
                                       traits_.make(villian.substr(2,2)));
                }
        private:
                long map_(long h0, long h1, long v0, long v1)const{
                        return h0 * 52*52*52 +
                               h1 * 52*52    +
                               v0 * 52       +
                               v1;
                }
                std::vector<equity_player> cache_;
                card_traits traits_;
        };

        void ec_test(){
                equity_calc ec;
                card_traits t;
                std::vector<equity_player> players;
                std::vector<long> board;




                /*
                
                +----+------+-------+------+
                |Hand|Equity| Wins  | Ties |
                +----+------+-------+------+
                |ahkh|50.08%|852,207|10,775|
                |2s2c|49.92%|849,322|10,775|
                +----+------+-------+------+

                */
                players.emplace_back( t.make("Ah"), t.make("Kh") );
                players.emplace_back( t.make("2d"), t.make("2c") );

                ec.calc( players );
                for( auto const& p : players){
                        std::cout << p << "\n";
                } 

                /*
                +----+------+-------+-----+
                |Hand|Equity| Wins  |Ties |
                +----+------+-------+-----+
                |ahkh|42.12%|574,928|7,155|
                |2s2c|25.22%|343,287|7,155|
                |5c6c|32.67%|445,384|7,155|
                +----+------+-------+-----+
                */
                players.clear();
                std::cout << "---------------------------------------\n";
                players.emplace_back( t.make("Ah"), t.make("Kh") );
                players.emplace_back( t.make("2d"), t.make("2c") );
                players.emplace_back( t.make("5c"), t.make("6c") );
                ec.calc( players );
                for( auto const& p : players){
                        std::cout << p << "\n";
                } 
                
                /*

                */
                players.clear();
                std::cout << "---------------------------------------\n";
                players.emplace_back( t.make("Ah"), t.make("Kh") );
                players.emplace_back( t.make("2s"), t.make("2c") );
                players.emplace_back( t.make("5c"), t.make("6c") );
                board = std::vector<long>{t.make( "8d"), t.make("9d"), t.make("js") };
                ec.calc( players, board);
                for( auto const& p : players){
                        std::cout << p << "\n";
                } 

        }
}

#endif // PS_EQUITY_CALC_H
