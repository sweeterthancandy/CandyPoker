#include "ps/algorithm.h"

#include <boost/range/algorithm.hpp>

namespace ps{

/*

   The idea of this is to 2-tuple of permutations (C,S), 
   so for a vector of n-players, we apply the permutation
                        C : c -> c',
   ie
                (p1,p2,p3) -> (p1', p2', p3'),
   and then apply the suit permutation S : s -> s'
                (p1', p2', p3') -> (p1'', p2'', p3''),
   so that given and set of players, we can can an injective
   mapping to a small subset. Ei, AA vs KK == KK vs AA 
   with the player orientation swapped around.
        Noting that we can complete ignore any permutation 
   of suits for all practical purposes, as always just want
   to find a suit permutation so that it's injective

                    eval(AA,KK vs AA,KK)
    
        =           eval(AA vs AA ) * w0 + 
                    eval(AA vs KK ) * w1 + 
                    eval(KK vs AA ) * w2 + 
                    eval(KK vs KK ) * w3
    
        =           eval(AA vs AA ) * w0 + 
                    eval(AA vs KK ) * w1 + 
            Inverse(eval(AA vs KK )) * w2 + 
                    eval(KK vs KK ) * w3

          
        
        

        
 */

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
