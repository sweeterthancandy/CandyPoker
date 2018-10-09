#include "ps/base/algorithm.h"

#include <boost/range/algorithm.hpp>
#include <map>

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
        // first create vector of n, and token_n = hh_n
        //      (0,hh_0), (1,hh_1), ... (n,hh_n),
        // where first h is greater handk the second h
        std::vector< std::tuple< size_t, std::string> > player_perm;
        for(size_t i=0;i!=players.size();++i){
                auto h =  holdem_hand_decl::get( players[i] ) ;
                player_perm.emplace_back(i, h.first().rank().to_string() +
                                            h.second().rank().to_string() );
        }
        // sort it by the token
        boost::sort(player_perm, [](auto const& left, auto const& right){
                return std::get<1>(left) < std::get<1>(right);
        });

        // new work out the perm used to create it
        std::vector<int> perm;
        for(size_t i=0;i!=players.size();++i){
                perm.emplace_back( std::get<0>(player_perm[i]) );
        }

        std::vector<std::vector<holdem_hand_decl> > decls;



        // now we allocate suits, starting with 0 etc
        std::array< int, 4> rev_suit_map{-1,-1,-1,-1};
        int suit_iter = 0; // using the fact we know suits \in {0,1,2,3}
        // allocate pocket pairs common types first

        // AA KK -> AaAb KcKa
        //       -> AaAb KcKb -> AbAa KcKb

        std::map<int, int> pp_count;
        for(size_t i=0;i!=players.size();++i){
                auto h =  holdem_hand_decl::get( players[perm[i]] ) ;
                if( h.first().rank() != h.second().rank()){
                        continue;
                }
                ++pp_count[h.first().suit()];
                ++pp_count[h.second().suit()];
        }

        for(size_t i=0;i!=players.size();++i){
                auto h =  holdem_hand_decl::get( players[perm[i]] ) ;

                auto a = &h.first();
                auto b = &h.second();
                // TODO pocket pair
                #if 1
                if( a->rank() == b->rank()){
                        if( pp_count[a->suit()] > pp_count[b->suit()] ){
                                std::swap(a,b);
                        }
                }
                #endif


                if(     rev_suit_map[a->suit()] == -1 )
                        rev_suit_map[a->suit()] = suit_iter++;
                if(     rev_suit_map[b->suit()] == -1 )
                        rev_suit_map[b->suit()] = suit_iter++;
        }

        // TODO remove this, unneeded
        for(size_t i=0;i != 4;++i){
                if(     rev_suit_map[i] == -1 )
                        rev_suit_map[i] = suit_iter++;
        }

        // crate map
        std::vector< int> suit_perms;
        for(size_t i=0;i != 4;++i){
                suit_perms.emplace_back(rev_suit_map[i]);
        }
        
        std::vector<ps::holdem_id> perm_hands;
        for(size_t i=0;i != players.size();++i){
                auto h =  holdem_hand_decl::get( players[perm[i]] ) ;

                #if 0
                if( h.first().rank() == h.second().rank() ){
                        if( suit_perms[h.first().suit()] > suit_perms[h.second().suit()] ){
                                perm_hands.emplace_back( 
                                        holdem_hand_decl::make_id(
                                                h.first().rank(),
                                                suit_perms[h.second().suit()],
                                                h.second().rank(),
                                                suit_perms[h.first().suit()]));
                        } else {
                                perm_hands.emplace_back( 
                                        holdem_hand_decl::make_id(
                                                h.first().rank(),
                                                suit_perms[h.first().suit()],
                                                h.second().rank(),
                                                suit_perms[h.second().suit()]));
                        } 
                } else{
                #endif
                        perm_hands.emplace_back( 
                                holdem_hand_decl::make_id(
                                        h.first().rank(),
                                        suit_perms[h.first().suit()],
                                        h.second().rank(),
                                        suit_perms[h.second().suit()]));
                #if 0
                }
                #endif

        }
        return std::make_tuple( perm, perm_hands);
}

} // ps
