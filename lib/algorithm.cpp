#include "ps/algorithm.h"

#include <boost/range/algorithm.hpp>

namespace ps{

std::tuple<
        std::vector<int>,
        std::vector<ps::holdem_id>
> permutate_for_the_better( std::vector<ps::holdem_id> const& players ){
        std::vector< std::tuple< size_t, std::string> > player_perm;
        for(size_t i=0;i!=players.size();++i){
                auto h{ holdem_hand_decl::get( players[i] ) };
                player_perm.emplace_back(i, h.first().rank().to_string() +
                                            h.second().rank().to_string() );
        }
        boost::sort(player_perm, [](auto const& left, auto const& right){
                return std::get<1>(left) < std::get<1>(right);
        });
        std::vector<int> perm;
        std::array< int, 4> suits{0,1,2,3};
        std::array< int, 4> rev_suit_map{-1,-1,-1,-1};
        int suit_iter = 0;

        std::stringstream from, to;
        for(size_t i=0;i!=players.size();++i){
                perm.emplace_back( std::get<0>(player_perm[i]) );
        }
        for(size_t i=0;i!=players.size();++i){
                auto h{ holdem_hand_decl::get( players[perm[i]] ) };

                // TODO pocket pair
                if(     rev_suit_map[h.first().suit()] == -1 )
                        rev_suit_map[h.first().suit()] = suit_iter++;
                if(     rev_suit_map[h.second().suit()] == -1 )
                        rev_suit_map[h.second().suit()] = suit_iter++;
        }
        for(size_t i=0;i != 4;++i){
                if(     rev_suit_map[i] == -1 )
                        rev_suit_map[i] = suit_iter++;
        }
        std::vector< int> suit_perms;
        for(size_t i=0;i != 4;++i){
                suit_perms.emplace_back(rev_suit_map[i]);
        }
        
        std::vector<ps::holdem_id> perm_hands;
        for(size_t i=0;i != players.size();++i){
                auto h{ holdem_hand_decl::get( players[perm[i]] ) };
                perm_hands.emplace_back( 
                        holdem_hand_decl::make_id(
                                h.first().rank(),
                                suit_perms[h.first().suit()],
                                h.second().rank(),
                                suit_perms[h.second().suit()]));
        }
        return std::make_tuple( perm, perm_hands);
}

} // ps
