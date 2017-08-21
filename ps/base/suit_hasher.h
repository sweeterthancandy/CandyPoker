#ifndef PS_BASE_SUIT_HASHER_H
#define PS_BASE_SUIT_HASHER_H

namespace ps{

struct suit_hasher{
        

        using hash_t = size_t;

        hash_t create()const noexcept{ return 1; }
        hash_t append(hash_t hash, rank_id rank)const noexcept{
                static constexpr const std::array<int,4> suit_map = { 2,3,5,7 };
                return hash * suit_map[rank];
        }
        template<class... Args>
        hash_t create_from_cards(Args... args)const noexcept{
                auto hash = create();
                int _[] = {0,  (hash = append(hash, card_decl::get(args).suit()),0)...};
                return hash;
        }
        bool has_flush(hash_t hash)const noexcept{
                if( hash == 0 )
                        return false;
                return 
                    ((hash % (2*2*2*2*2))*
                     (hash % (3*3*3*3*3))*
                     (hash % (5*5*5*5*5))*
                     (hash % (7*7*7*7*7))) == 0;
        }
};

} // ps

#endif // PS_BASE_SUIT_HASHER_H
