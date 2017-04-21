#include "ps/equity_calc.h"


#include "ps/detail/visit_combinations.h"

#include <boost/format.hpp>

namespace ps{
                
bool equity_calc::run( numeric::result_type& result,
                       std::vector<holdem_id> const& players,
                       std::vector<card_id> const& board,
                       std::vector<card_id> const& dead)noexcept
{
        switch( players.size()){
        case 2: return run_p<2>( players, board, dead, result ); break;
        case 3: return run_p<3>( players, board, dead, result ); break;
        case 4: return run_p<4>( players, board, dead, result ); break;
        case 5: return run_p<5>( players, board, dead, result ); break;
        case 6: return run_p<6>( players, board, dead, result ); break;
        case 7: return run_p<7>( players, board, dead, result ); break;
        case 8: return run_p<8>( players, board, dead, result ); break;
        case 9: return run_p<9>( players, board, dead, result ); break;
        default:
                return false;
        }
}
                
template<size_t Num_Players>
bool equity_calc::run_p( std::vector<holdem_id> const& players,
            std::vector<card_id> const& board,
            std::vector<card_id> const& dead,
            numeric::result_type& result)noexcept
{
        auto dealt{ board.size() + dead.size() };
        auto to_deal{ 5- dealt  };
        switch(to_deal){
        case 1: return run_pd<Num_Players, 1>( players, board, dead, result);
        case 2: return run_pd<Num_Players, 2>( players, board, dead, result);
        case 3: return run_pd<Num_Players, 3>( players, board, dead, result);
        case 4: return run_pd<Num_Players, 4>( players, board, dead, result);
        case 5: return run_pd<Num_Players, 5>( players, board, dead, result);
        default:
                return false;
        }
}
template<size_t Num_Players, size_t Num_Deal>
bool equity_calc::run_pd( std::vector<holdem_id> const& players,
            std::vector<card_id> const& board,
            std::vector<card_id> const& dead,
            numeric::result_type& result)noexcept{

        std::vector<id_type> known;

        // cache the cards
        std::array<id_type, Num_Players> x;
        std::array<id_type, Num_Players> y;

        size_t sigma{0};
        std::array<size_t,Num_Players> wins;
        std::array<size_t,Num_Players> draws;
        std::array<double,Num_Players> equity;

        for( size_t i{0}; i!= Num_Players;++i){
                auto const& p{ players[i]};
                x[i]      = holdem_hand_decl::get(players[i]).first().id();
                y[i]      = holdem_hand_decl::get(players[i]).second().id();
                wins[i]   = 0;
                draws[i]  = 0;
                equity[i] = 0.0;
        }

        boost::copy( x, std::back_inserter(known));
        boost::copy( y, std::back_inserter(known));
        boost::copy( board, std::back_inserter(known));
        boost::copy( dead , std::back_inserter(known));
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

        for(size_t i{0};i!=Num_Players;++i){
                using tag = numeric::result_type::Tags;
                result.nat_mat(i, tag::NTag_Wins)   = wins[i];
                result.nat_mat(i, tag::NTag_Draws)  = draws[i];
                result.nat_mat(i, tag::NTag_Sigma)  = sigma;
                result.rel_mat(i, tag::RTag_Equity) = equity[i];
        }
        return true;
}

} // ps
