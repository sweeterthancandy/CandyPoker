#ifndef PS_BASE_HASH_H
#define PS_BASE_HASH_H

#include <cstdint>
#include <array>

#include "ps/support/config.h"
#include "ps/base/cards_fwd.h"
#include "ps/base/cards_intrinsic.h"
#include "ps/base/rank_vector.h"

namespace ps{

using suit_hash_t = std::uint32_t;

inline
suit_hash_t suit_hash_create()noexcept{
        return static_cast<suit_hash_t>(1);
}
inline
suit_hash_t suit_hash_append(suit_hash_t hash, rank_id rank)noexcept{
        static constexpr const std::array<suit_id,4> suit_map = { 2,3,5,7 };
        return hash * suit_map[rank];
}

template<class... Args>
suit_hash_t suit_hash_create_from_cards(Args... args)noexcept{
        auto hash = suit_hash_create();
        int dummy[] = {0,  (hash = suit_hash_append(hash, card_suit_from_id(args)),0)...};
        return hash;
}
inline
bool suit_hash_has_flush(suit_hash_t hash)noexcept{
        if( hash == 0 )
                return false;
        return 
            ((hash % (2*2*2*2*2))*
             (hash % (3*3*3*3*3))*
             (hash % (5*5*5*5*5))*
             (hash % (7*7*7*7*7))) == 0;
}

using rank_hash_t = std::uint32_t;

/*
          +----+--+--+--+--+--+--+--+--+--+--+--+--+--+
          |card|A |K |Q |J |T |9 |8 |7 |6 |5 |4 |3 |2 |
          +----+--+--+--+--+--+--+--+--+--+--+--+--+--+
          |yyyy|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|
          +----+--+--+--+--+--+--+--+--+--+--+--+--+--+
          |  1A|18|16|14|12|10| E| C| A| 8| 6| 4| 2| 0|
          +----+--+--+--+--+--+--+--+--+--+--+--+--+--+

          yyyy ~ value of rank with 4 cards, zero 
                 when there warn't 4 cards

          xx   ~ bit mask to non-injective mapping for
                 number of cards, 


                           n | bits
                           --+-----
                           0 | 00
                           1 | 01
                           2 | 10
                           3 | 11
                           4 | 11
*/
inline
rank_hash_t rank_hash_append(rank_hash_t hash, rank_id rank)noexcept{
        auto idx = rank * 2;
        auto mask = ( hash & ( 0x3 << idx ) ) >> idx;
        switch(mask){
        case 0x0:
                // set the idx'th bit
                hash |= 0x1 << idx;
                break;
        case 0x1:
                // unset the idx'th bit
                hash &= ~(0x1 << idx);
                // set the (idx+1)'th bit
                hash |= 0x1 << (idx+1);
                break;
        case 0x2:
                // set the idx'th bit
                hash |= 0x1 << idx;
                break;
        case 0x3:
                // set the special part of mask for the fourth card
                hash |= (rank + 1) << 0x1A;
                // zero out for ease of reverse parsing
                hash &= ~(0x3 << idx);
                break;
        default:
                PS_UNREACHABLE();
        }
        return hash;
}
inline
rank_vector rank_hash_get_vector(rank_hash_t hash)noexcept{
        rank_vector ret;

        auto fourth = hash >> 0x1A;
        if( fourth ){
                ret.push_back( fourth -1 );
        }
        for(size_t rank=0;rank!=13;++rank){
                auto idx = rank * 2;
                auto mask = ( hash & ( 0x3 << idx ) ) >> idx;
                for(;mask;--mask){
                        ret.push_back(rank);
                }
        }
        return std::move(ret);
}

inline
rank_hash_t rank_hash_create()noexcept{ return 0; }
inline
rank_hash_t rank_hash_create(rank_vector const& rv)noexcept{
        auto hash = rank_hash_create();
        for(auto id : rv )
                hash = rank_hash_append(hash, id);
        return hash;
}
template<class... Args>
rank_hash_t rank_hash_create(Args... args)noexcept{
        auto hash = rank_hash_create();
        int _[] = {0,  (hash = rank_hash_append(hash, args),0)...};
        return hash;
}
template<class... Args>
rank_hash_t rank_hash_create_from_cards(Args... args)noexcept{
        auto hash = rank_hash_create();
        int _[] = {0,  (hash = rank_hash_append(hash, card_rank_from_id(args)),0)...};
        return hash;
}
inline
const rank_hash_t rank_hash_max(size_t n = 7)noexcept{
        switch(n){
        case 7:
        default:
                return rank_hash_create(12,12,12,12,11,11,11);
        }
}

using card_hash_t = std::uint64_t;

inline
card_hash_t card_hash__detail__pack(suit_hash_t sh, rank_hash_t rh)noexcept{
        return 
                (static_cast<card_hash_t>(sh) << 32) |
                 static_cast<card_hash_t>(rh)
        ;
}
inline
suit_hash_t card_hash_get_suit(card_hash_t hash)noexcept{
        return static_cast<suit_hash_t>( hash >> 32 ); 
}
inline
rank_hash_t card_hash_get_rank(card_hash_t hash)noexcept{
        return static_cast<rank_hash_t>( hash & 0xffffffff); 
}
inline
card_hash_t card_hash_create()noexcept{
        return card_hash__detail__pack(
                suit_hash_create(),
                rank_hash_create());
}
inline
card_hash_t card_hash_append(card_hash_t hash, card_id id)noexcept{
        return card_hash__detail__pack(
                suit_hash_append( card_hash_get_suit(hash), card_suit_from_id(id)),
                rank_hash_append( card_hash_get_rank(hash), card_rank_from_id(id)));
}
inline
card_hash_t card_hash_append_2(card_hash_t hash, card_id x, card_id y)noexcept{
        auto sh = 
                suit_hash_append(
                        suit_hash_append(
                                card_hash_get_suit(hash),
                                card_suit_from_id(x)
                        ),
                        card_suit_from_id(y)
                );
        auto rh = 
                rank_hash_append(
                        rank_hash_append(
                                card_hash_get_rank(hash),
                                card_rank_from_id(x)
                        ),
                        card_rank_from_id(y)
                );


        return card_hash__detail__pack(sh, rh);
}
template<class... Args>
card_hash_t card_hash_create_from_cards(Args... cards)noexcept{
        return card_hash__detail__pack(
                suit_hash_create_from_cards(cards...),
                rank_hash_create_from_cards(cards...));
}
inline
card_hash_t card_hash_create_from_cards(card_vector const& cards)noexcept{
        auto hash = card_hash_create();
        for( auto id : cards){
                hash = card_hash_append(hash, id);
        }
        return hash;
}

} // ps

#endif // PS_BASE_HASH_H
