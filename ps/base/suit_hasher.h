#ifndef PS_BASE_SUIT_HASHER_H
#define PS_BASE_SUIT_HASHER_H

namespace ps{

namespace suit_hasher{
        

        using suit_hash_t = size_t;

        inline
        suit_hash_t create()noexcept{ return 1; }
        inline
        suit_hash_t append(suit_hash_t hash, rank_id rank)noexcept{
                static constexpr const std::array<int,4> suit_map = { 2,3,5,7 };
                return hash * suit_map[rank];
        }
        template<class... Args>
        suit_hash_t create_from_cards(Args... args)noexcept{
                auto hash = create();
                int _[] = {0,  (hash = append(hash, card_suit_from_id(args)),0)...};
                return hash;
        }
        inline
        bool has_flush_unsafe(suit_hash_t hash)noexcept{
                return 
                    ((hash % (2*2*2*2*2))*
                     (hash % (3*3*3*3*3))*
                     (hash % (5*5*5*5*5))*
                     (hash % (7*7*7*7*7))) == 0;
        }
        inline
        bool has_flush(suit_hash_t hash)noexcept{
                if( hash == 0 )
                        return false;
                return has_flush_unsafe(hash);
        }
        inline suit_hash_t five_card_max()noexcept{
                return 37 * 37 * 37 * 37 * 31;
        }
} // end namespace suit_hasher
} // end namespace ps

#endif // PS_BASE_SUIT_HASHER_H
