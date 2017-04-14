#ifndef PS_EQUITY_CALC_H
#define PS_EQUITY_CALC_H

#include <ostream>
#include <string>

#include <boost/format.hpp>

#include "ps/core/eval.h"
#include "ps/detail/visit_combinations.h"
#include "ps/holdem/holdem_traits.h"


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
                using hand_type = holdem_traits::hand_type;
                equity_player(hand_type const& hand)
                        :hand_(hand),
                        wins_{0},draw_{0},sigma_{0},equity_{0.0}
                {}

                friend std::ostream& operator<<(std::ostream& ostr, equity_player const& self){
                        auto pct{ 1.0 / self.sigma_ * 100 };
                        return ostr  << boost::format("{ sigma=%d, wins=%d(%.2f), draw=%d(%.2f), equity=%.2f(%.2f) }")
                                % self.sigma_
                                % self.wins_ % ( self.wins_ * pct)
                                % self.draw_ % ( self.draw_ * pct)
                                % self.equity_ % ( self.equity_ * pct);
                }
                auto wins()const{ return wins_; }
                auto draws()const{ return draw_; }
                auto sigma()const{ return sigma_; }
                auto equity()const{ return equity_ / sigma_; }
        private:
                // these 2 classes a deeply coupled
                friend class equity_calc;

                hand_type hand_;


                size_t wins_;
                size_t draw_;
                size_t sigma_;
                double equity_;
        };
        
        struct equity_context{
                using set_type = holdem_traits::set_type;

                template<class... Args>
                equity_context& add_player(Args&&... args){
                        players_.emplace_back( hm_.make(std::forward<Args>(args)...) );
                        return *this;
                }
                template<class... Args>
                equity_context& add_board(Args&&... args){
                        boost::copy( hm_.make_set(std::forward<Args>(args)...), std::back_inserter(board_) );
                        return *this;
                }
                std::vector<equity_player>& get_players(){ return players_; }
                decltype(auto) get_board(){ return board_; }
                decltype(auto) get_dead(){ return dead_; }
        private:
                holdem_hand_maker hm_;

                set_type board_;
                set_type dead_;
                std::vector<equity_player> players_;
        };


        struct equity_calc{
                void run( equity_context& ctx){

                        std::vector<long> known;
                        for( auto const& p : ctx.get_players() ){
                                known.emplace_back(p.hand_[0]);
                                known.emplace_back(p.hand_[1]);
                        }
                        boost::copy( ctx.get_board(), std::back_inserter(known));
                        boost::copy( ctx.get_dead(), std::back_inserter(known));
                        boost::sort(known);
                        auto filter = [&](long c){ return ! boost::binary_search(known, c); };
                
                        auto do_eval = [&](long a, long b, long c, long d, long e){
                                std::vector<std::pair<std::uint32_t, equity_player* > > ranked;
                                std::vector<equity_player*> winners;
                                for( auto& p : ctx.get_players() ){
                                        ++p.sigma_;
                                        ranked.emplace_back( std::make_pair(eval_(p.hand_[0], p.hand_[1], a,b,c,d,e), &p));
                                }
                                boost::sort( ranked, [](auto const& l, auto const& r){ return l.first < r.first; });

                                auto iter{ boost::find_if( ranked, [&](auto const& _){ return _.first != ranked.front().first; } ) }; 
                                ranked.resize( iter - ranked.begin());
                                if( ranked.size() == 1 ){
                                        ++ranked.front().second->wins_;
                                        ranked.front().second->equity_ += 1.0;
                                } else{
                                        for( auto& r : ranked ){
                                                ++r.second->draw_;
                                                r.second->equity_ += 1.0 / ranked.size();
                                        }
                                }
                        };

                        auto const& board{ ctx.get_board() };


                        switch(ctx.get_board().size()){
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
        };

        #if 0
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
        #endif
}

#endif // PS_EQUITY_CALC_H
