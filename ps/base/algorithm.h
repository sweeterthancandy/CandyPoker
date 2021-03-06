/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_ALGORITHM_H
#define PS_ALGORITHM_H

#include <tuple>
#include <vector>
#include <type_traits>

#include "ps/base/cards.h"
#include "ps/support/array_view.h"

namespace ps{
        
template<class... Args
#if 0
	,
         class _ = detail::void_t<
                 std::enable_if_t<
                        std::is_same<std::decay_t<Args>, holdem_hand_decl>::value>...
        >
#endif
>
inline bool disjoint( Args&&... args){
        std::array< holdem_hand_decl const*, sizeof...(args)> aux{ &args...};
        std::set<card_id> s;
        for( auto ptr : aux ){
                s.insert( ptr->first() );
                s.insert( ptr->second() );
        }
        return s.size() == aux.size()*2;
}
        
#if 0
template<size_t N>
std::array<int, N> injective_player_perm( support::array_view<ps::holdem_id> const& players ){
        // first create vector of n, and token_n = hh_n
        //      (0,hh_0), (1,hh_1), ... (n,hh_n),
        // where first h is greater handk the second h
        std::array< std::tuple< size_t, size_t>, N> player_perm;
        for(size_t i=0;i!=players.size();++i){
                auto h =  holdem_hand_decl::get( players[i] ) ;
                player_perm[i] = std::make_tuple(i, 
                                                 h.first().rank().id() * 17 + 
                                                 h.second().rank().id());
        }
        // sort it by the token
        boost::sort(player_perm, [](auto const& left, auto const& right){
                return std::get<1>(left) < std::get<1>(right);
        });

        // new work out the perm used to create it
        std::array<int, N> perm;
        for(size_t i=0;i!=players.size();++i){
                perm[i] = std::get<0>(player_perm[i]);
        }
        return std::move(perm);
}

inline
std::array<int, 4> injective_suit_perm( support::array_view<ps::holdem_id> const& players ){

        std::array< int, 4> rev_suit_map{-1,-1,-1,-1};
        int suit_iter = 0; // using the fact we know suits \in {0,1,2,3}
        for(size_t i=0;i!=players.size();++i){
                auto h =  holdem_hand_decl::get( players[i] ) ;

                // TODO pocket pair
                if(     rev_suit_map[h.first().suit()] == -1 )
                        rev_suit_map[h.first().suit()] = suit_iter++;
                if(     rev_suit_map[h.second().suit()] == -1 )
                        rev_suit_map[h.second().suit()] = suit_iter++;
        }

        #if 0
        // TODO remove this, unneeded
        for(size_t i=0;i != 4;++i){
                if(     rev_suit_map[i] == -1 )
                        rev_suit_map[i] = suit_iter++;
        }
        #endif

        return rev_suit_map;
}

template<size_t N>
std::tuple<
        std::array<int, N>,
        std::array<ps::holdem_id, N>
> permutate_for_the_better( support::array_view<ps::holdem_id> const& players ){
        
        std::array<int, N> perm{ injective_player_perm<N>( players) };
        std::array<ps::holdem_id, N> pplayers;
        for(int i=0;i!=N;++i)
                pplayers[i] = players[perm[i]];


        std::array<int, 4> suit_perm{ injective_suit_perm( pplayers) };
        
        std::array<ps::holdem_id, N> perm_hands;
        for(size_t i=0;i != players.size();++i){
                auto h =  holdem_hand_decl::get( players[perm[i]] ) ;
                perm_hands[i] = 
                        holdem_hand_decl::make_id(
                                h.first().rank(),
                                suit_perm[h.first().suit()],
                                h.second().rank(),
                                suit_perm[h.second().suit()]);
        }
        return std::make_tuple( perm, perm_hands);
}
#endif 


std::tuple<
        std::vector<int>,
        std::vector<ps::holdem_id>
> permutate_for_the_better( std::vector<ps::holdem_id> const& players );

#if 0
std::tuple<
        std::vector<int>,
        holdem_hand_vector
> permutate_for_the_better( holdem_hand_vector const& players );
#endif
} // ps

#endif // PS_ALGORITHM_H
