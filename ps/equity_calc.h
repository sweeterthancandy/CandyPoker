#ifndef PS_EQUITY_CALC_H
#define PS_EQUITY_CALC_H

#include <ostream>
#include <string>

#include <boost/format.hpp>

#include "ps/eval.h"
#include "ps/detail/visit_combinations.h"
#include "ps/cards.h"
#include "ps/frontend.h"


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
                equity_player(id_type hand)
                        :hand_{holdem_hand_decl::get(hand)},
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

                auto get_hand()const{ return hand_; }
        private:
                // these 2 classes a deeply coupled
                friend class equity_calc;

                holdem_hand_decl const& hand_;

                size_t wins_;
                size_t draw_;
                size_t sigma_;
                double equity_;
        };
        
        struct equity_context{

		equity_context& add_player( frontend::hand hand){
                        players_.emplace_back( hand.get() );
                        return *this;
		}
                equity_context& add_player(std::string const& s){
                        players_.emplace_back( holdem_hand_decl::get(s).id() );
                        return *this;
                }
                equity_context& add_board(std::string const& s){
                        for( size_t i=0; i!= s.size();i+=2)
                                board_.emplace_back( card_decl::get(s.substr(i,2)).id() );
                        return *this;
                }
                std::vector<equity_player>& get_players(){ return players_; }
                decltype(auto) get_board(){ return board_; }
                decltype(auto) get_dead(){ return dead_; }
        private:

                std::vector<id_type> board_;
                std::vector<id_type> dead_;

                std::vector<equity_player> players_;
        };


        struct equity_calc{
                void run( equity_context& ctx){
                        switch( ctx.get_players().size()){
                        case 2: do_run<2>( ctx ); break;
                        case 3: do_run<3>( ctx ); break;
                        case 4: do_run<4>( ctx ); break;
                        case 5: do_run<5>( ctx ); break;
                        case 6: do_run<6>( ctx ); break;
                        case 7: do_run<7>( ctx ); break;
                        case 8: do_run<8>( ctx ); break;
                        case 9: do_run<9>( ctx ); break;
                        default: BOOST_THROW_EXCEPTION(std::domain_error("bad number of players"));
                        }
                }
        private:
                template<size_t Num_Players>
                void do_run(equity_context& ctx){


                        std::vector<id_type> known;

                        // cache the cards
                        std::array<id_type, Num_Players> x;
                        std::array<id_type, Num_Players> y;

                        size_t sigma{0};
                        std::array<size_t,Num_Players> wins;
                        std::array<size_t,Num_Players> draws;
                        std::array<double,Num_Players> equity;

                        for( size_t i{0}; i!= Num_Players;++i){
                                auto const& p{ ctx.get_players()[i]};
                                x[i] = p.get_hand().first().id();
                                y[i] = p.get_hand().second().id(); 
                                wins[i] = 0;
                                draws[i] = 0;
                                equity[i] = 0.0;
                        }

                        boost::copy( x, std::back_inserter(known));
                        boost::copy( y, std::back_inserter(known));
                        boost::copy( ctx.get_board(), std::back_inserter(known));
                        boost::copy( ctx.get_dead(), std::back_inserter(known));
                        boost::sort(known);
                        auto filter = [&](long c){ return ! boost::binary_search(known, c); };
                
                        auto do_eval = [&](id_type a, id_type b, id_type c, id_type d, id_type e){
                                std::array<std::pair<std::uint32_t, size_t >, Num_Players > ranked;
                                ++sigma;
                                for(size_t i{0};i!=Num_Players;++i){
                                        ranked[i] = std::make_pair(eval_(x[i],y[i],a,b,c,d,e), i);
                                }
                                boost::sort( ranked, [](auto const& l, auto const& r){ return l.first < r.first; });
                                auto winning_rank{ ranked.front().first };
                                auto iter{ boost::find_if( ranked, [&](auto const& _){ return _.first != winning_rank; } ) }; 
                                auto num_winners{ std::distance( ranked.begin(), iter) };
                                if( num_winners == 1 ){
                                        ++wins[ranked.front().second];
                                        equity[ranked.front().second] += 1.0;
                                } else{
                                        for( auto j{ ranked.begin() }; j!=iter;++j){
                                                ++draws[j->second];
                                                equity[j->second] += 1.0 / num_winners;
                                        }
                                }
                        };

                        auto const& board{ ctx.get_board() };


                        switch(ctx.get_board().size()){
                        case 0:
                                detail::visit_combinations<5>( [&](id_type a, id_type b, id_type c, id_type d, id_type e){
                                        do_eval(a,b,c,d,e);
                                }, filter, 51);
                                break;
                        case 1:
                                detail::visit_combinations<4>( [&](id_type a, id_type b, id_type c, id_type d){
                                        do_eval(board[0], a,b,c,d);
                                }, filter, 51);
                                break;
                        case 2:
                                detail::visit_combinations<3>( [&](id_type a, id_type b, id_type c){
                                        do_eval(board[0], board[1], a,b,c);
                                }, filter, 51);
                                break;
                        case 3:
                                detail::visit_combinations<2>( [&](id_type a, id_type b){
                                        do_eval(board[0], board[1], board[2], a,b);
                                }, filter, 51);
                                break;
                        case 4:
                                detail::visit_combinations<1>( [&](id_type a){
                                        do_eval(board[0], board[1], board[2], board[3], a);
                                }, filter, 51);
                                break;
                        case 5:
                                do_eval( board[0], board[1], board[2], board[3], board[4]);
                                break;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("bad number of board cards"));
                        }

                        for(size_t i{0};i!=Num_Players;++i){
                                ctx.get_players()[i].wins_   = wins[i];
                                ctx.get_players()[i].draw_   = draws[i];
                                ctx.get_players()[i].equity_ = equity[i];
                                ctx.get_players()[i].sigma_  = sigma;
                        }
                }
        private:
                eval eval_;
        };

}

#endif // PS_EQUITY_CALC_H
