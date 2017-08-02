#ifndef PS_EQUITY_CALC_DETAIL_H
#define PS_EQUITY_CALC_DETAIL_H

#include "ps/base/cards.h"
#include "ps/eval/eval.h"

#include "ps/detail/visit_combinations.h"

#include <boost/format.hpp>
#include <boost/range/algorithm.hpp>

namespace ps{

        /*
                The point of this class is provide an efficent way of enumerating
                a board runout of n-players.
         */
        struct equity_calc_detail{

                template<
                        class Visitor,
                        class Players_Type
                >
                bool visit_boards( Visitor& v, Players_Type const& players){
                        static std::array<card_id,0> aux;
                        return visit_boards(v, players, aux, aux);
                }
                template<
                        class Visitor,
                        class Players_Type,
                        class Board_Type,
                        class Dead_Type
                >
                bool visit_boards( Visitor& v,
                                   Players_Type const& players,
                                   Board_Type const& board,
                                   Dead_Type const& dead)noexcept
                {
                        switch( players.size()){
                        case 2: return visit_boards_p<2>( v, players, board, dead ); break;
                        case 3: return visit_boards_p<3>( v, players, board, dead ); break;
                        case 4: return visit_boards_p<4>( v, players, board, dead ); break;
                        case 5: return visit_boards_p<5>( v, players, board, dead ); break;
                        case 6: return visit_boards_p<6>( v, players, board, dead ); break;
                        case 7: return visit_boards_p<7>( v, players, board, dead ); break;
                        case 8: return visit_boards_p<8>( v, players, board, dead ); break;
                        case 9: return visit_boards_p<9>( v, players, board, dead ); break;
                        default:
                                return false;
                        }
                }
                template<
                        size_t Num_Players,
                        class Visitor,
                        class Players_Type,
                        class Board_Type,
                        class Dead_Type
                >
                bool visit_boards_p( Visitor& v,
                                     Players_Type const& players,
                                     Board_Type const& board,
                                     Dead_Type const& dead)noexcept
                {
                        auto dealt =  board.size() + dead.size() ;
                        auto to_deal =  5- dealt  ;
                        switch(to_deal){
                        case 1: return visit_boards_pd<Num_Players, 1>(v, players, board, dead);
                        case 2: return visit_boards_pd<Num_Players, 2>(v, players, board, dead);
                        case 3: return visit_boards_pd<Num_Players, 3>(v, players, board, dead);
                        case 4: return visit_boards_pd<Num_Players, 4>(v, players, board, dead);
                        case 5: return visit_boards_pd<Num_Players, 5>(v, players, board, dead);
                        default:
                                return false;
                        }
                }
                template<
                        size_t Num_Players,
                        size_t Num_Deal,
                        class Visitor,
                        class Players_Type,
                        class Board_Type,
                        class Dead_Type
                >
                bool visit_boards_pd( Visitor& v,
                                      Players_Type const& players,
                                      Board_Type const& board,
                                      Dead_Type const& dead)noexcept
                {
                        std::vector<id_type> known;

                        // cache the cards
                        std::array<id_type, Num_Players> x;
                        std::array<id_type, Num_Players> y;

                        for( size_t i{0}; i!= Num_Players;++i){
                                auto const& p =  players[i];
                                x[i]      = holdem_hand_decl::get(players[i]).first().id();
                                y[i]      = holdem_hand_decl::get(players[i]).second().id();
                        }

                        boost::copy( x, std::back_inserter(known));
                        boost::copy( y, std::back_inserter(known));
                        boost::copy( board, std::back_inserter(known));
                        boost::copy( dead , std::back_inserter(known));
                        boost::sort(known);
                        auto filter = [&](long c){ return ! boost::binary_search(known, c); };

                        auto do_eval = [&](id_type a, id_type b, id_type c, id_type d, id_type e){
                                std::array<std::uint32_t, Num_Players > ranked;
                                for(size_t i{0};i!=Num_Players;++i){
                                        ranked[i] = eval_(x[i],y[i],a,b,c,d,e);
                                }
                                v(a,b,c,d,e, ranked);
                        };
                        #if 0
                        auto do_eval = [&](id_type a, id_type b, id_type c, id_type d, id_type e){
                                std::array<std::pair<std::uint32_t, size_t >, Num_Players > ranked;
                                ++sigma;
                                for(size_t i{0};i!=Num_Players;++i){
                                        ranked[i] = std::make_pair(eval_(x[i],y[i],a,b,c,d,e), i);
                                }
                                boost::sort( ranked, [](auto const& l, auto const& r){ return l.first < r.first; });
                                auto winning_rank =  ranked.front().first ;
                                auto iter{ boost::find_if( ranked, [&](auto const& _){ return _.first != winning_rank; } ) }; 
                                auto num_winners =  std::distance( ranked.begin(), iter) ;

                                for( auto j{ ranked.begin() }; j!=iter;++j){
                                        ++win_matrix[j->second][num_winners - 1];
                                        equity[j->second] += 1.0 / num_winners;
                                }
                        };
                        #endif

                        switch(board.size()){
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

                        #if 0
                        if( result.size1() != Num_Players ||
                            result.size2() != computation_size ){
                                result = bnu::matrix<size_t>{Num_Players, computation_size, 0};
                        }

                        for(size_t i{0};i!=Num_Players;++i){
                                for(size_t j=0;j!= win_matrix[i].size();++j){
                                        result(i, j)   = win_matrix[i][j];
                                }
                                result(i,9) = sigma;
                                result(i,10) = equity[i] * computation_equity_fixed_prec;
                        }
                        #endif
                        return true;
                }
                eval eval_;
        };

} // ps

#endif // PS_EQUITY_CALC_DETAIL_H
