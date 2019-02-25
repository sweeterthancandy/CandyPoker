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
#include "ps/base/algorithm.h"
#include "ps/base/rank_hasher.h"

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
struct rank_info{
        size_t index;
        holdem_id hid;
        rank_hasher::rank_hash_t rank_hash;
        bool is_suited;
        friend bool operator==(rank_info const& l, rank_info const& r)noexcept{
                return ( ! ( l < r ) ) && ( ! ( r < l ) );
        }
        friend bool operator!=(rank_info const& l, rank_info const& r)noexcept{
                return ( ! (l == r) );
        }
        friend bool operator<(rank_info const& l, rank_info const& r)noexcept{
                if( l.rank_hash != r.rank_hash ){
                        return l.rank_hash <  r.rank_hash;
                }
                return l.is_suited <  r.is_suited;
        }
};
struct suit_perm_info{
        std::array<suit_id, 4> suit_perm;
        std::vector<int> player_perm;
        holdem_hand_vector hv;
        size_t card_mask;
        friend bool operator<(suit_perm_info const& l, suit_perm_info const& r)noexcept{
                #if 0
                if( l.card_mask != r.card_mask )
                        return l.card_mask < r.card_mask;
                #endif
                return l.hv < r.hv;
        }
};


std::tuple<
        std::vector<int>,
        std::vector<holdem_id>
> permutate_for_the_better( std::vector<holdem_id> const& players )
{
        enum{ Debug = false };

        std::vector< rank_info > ri;
        for(size_t idx=0;idx!=players.size();++idx){
                auto h =  holdem_hand_decl::get( players[idx] ) ;
                auto rank_hash = rank_hasher::create_from_cards(h.first(), h.second());
                ri.emplace_back(rank_info{idx, players[idx], rank_hash, h.is_suited()});
        }
        boost::sort( ri );

        std::vector< std::vector<rank_info> > level_sets;

        level_sets.emplace_back();
        level_sets.back().push_back( ri[0] );
        for(size_t idx=1; idx < ri.size();++idx){
                if( level_sets.back().front() == ri[idx]){
                        // same set
                        level_sets.back().push_back(ri[idx]);
                } else {
                        // start new set
                        level_sets.emplace_back();
                        level_sets.back().push_back(ri[idx]);
                }
        }

        if( Debug ){
                PS_LOG(trace) << "begin level_sets";
                for(auto const& ls : level_sets ){
                        holdem_hand_vector hv;
                        for(auto const& _ : ls){
                                hv.push_back(_.hid);
                        }
                        PS_LOG(trace) << "    -" << hv;
                }
                PS_LOG(trace) << "end   level_sets";
        }

        std::vector< std::vector< rank_info > > rank_permutations;
        rank_permutations.emplace_back();

        for(auto& ls : level_sets){
                if( ls.size() == 1 ){
                        for(auto& _ : rank_permutations ){
                                _.push_back(ls.back());
                        }
                } else {
                        
                        decltype(rank_permutations) next;

                        // should be sorted anyway
                        boost::sort(ls);
                        do{
                                for(auto rp : rank_permutations ){
                                        for(auto const& item : ls ){
                                                rp.push_back(item);
                                        }
                                        next.push_back(rp);
                                }
                        }while(boost::next_permutation(ls, [](auto const& l, auto const& r){ return l.index < r.index; }));
                        rank_permutations = std::move(next);
                }
        }

        if( Debug ){
                PS_LOG(trace) << "rank_permutations.size() => " << rank_permutations.size();
                PS_LOG(trace) << "begin rank_permutations";
                for(auto const& rp : rank_permutations ){
                        holdem_hand_vector hv;
                        for(auto const& _ : rp){
                                hv.push_back(_.hid);
                        }
                        PS_LOG(trace) << "    -" << hv;
                }
                PS_LOG(trace) << "end   rank_permutations";
        }
        
        std::vector<suit_perm_info> suit_perm_vec;

        std::array<suit_id, 4> suits = { 0, 1, 2, 3};
        for(auto const& rp : rank_permutations ){
                boost::sort(suits);
                do{
                        std::vector<int> player_perm;
                        holdem_hand_vector hv;
                        size_t mask = 0;

                        for(auto const& _ : rp){

                                auto h =  holdem_hand_decl::get( _.hid ) ;

                                rank_id r0 = h.first().rank();
                                suit_id s0 = h.first().suit();
                                rank_id r1 = h.second().rank();
                                suit_id s1 = h.second().suit();

                                suit_id m0 = suits[s0];
                                suit_id m1 = suits[s1];

                                holdem_id mhid = holdem_hand_decl::make_id(r0, m0, r1, m1);

                                mask |= static_cast<size_t>(1) << mhid;

                                player_perm.push_back(_.index);
                                hv.push_back(mhid);
                        }

                        suit_perm_vec.push_back(suit_perm_info{suits, std::move(player_perm),
                                                               hv, mask});

                }while(boost::next_permutation(suits));
        }

        boost::sort(suit_perm_vec);
        if( Debug ){
                PS_LOG(trace) << "begin rank_permutations";
                for(auto const& sp : suit_perm_vec ){
                        PS_LOG(trace) << "    -" << sp.hv;
                }
                PS_LOG(trace) << "end   rank_permutations";
                PS_LOG(trace) << "hv = " << suit_perm_vec.front().hv;
        }

        return { suit_perm_vec.front().player_perm, suit_perm_vec.front().hv };

}

#if 0
std::tuple<
        std::vector<int>,
        std::vector<holdem_id>
> permutate_for_the_better( std::vector<holdem_id> const& players ){
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
        
        std::vector<holdem_id> perm_hands;
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
#endif

} // ps
