#ifndef PS_BASE_SUIT_HASHER_H
#define PS_BASE_SUIT_HASHER_H

namespace ps{

namespace suit_hasher{
        

        using suit_hash_t = size_t;

        inline
        suit_hash_t append(suit_hash_t hash, rank_id rank)noexcept{
                static constexpr const std::array<int,4> suit_map = { 2,3,5,7 };
                return hash * suit_map[rank];
        }
        inline
        suit_hash_t create()noexcept{ return 1; }
        template<class... Args>
        suit_hash_t create(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, args),0)...};
                return val;
        }
        template<class... Args>
        suit_hash_t create_from_cards(Args... args)noexcept{
                auto hash = create();
                int _[] = {0,  (hash = append(hash, card_suit_from_id(args)),0)...};
                return hash;
        }
        inline
        suit_hash_t create(card_vector const& cv) noexcept{
                auto hash = create();
                for(auto id : cv )
                        hash = append(hash, card_suit_from_id(id));
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
        inline bool is_five_card_flush(suit_hash_t hash)noexcept{
                switch(hash){
                case 2*2*2*2*2:
                case 3*3*3*3*3:
                case 5*5*5*5*5:
                case 7*7*7*7*7:
                        return true;
                default:
                        return false;
                }
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


#if 0
// Ive kept a similar implementation because I found a large performance difference
namespace ps{
namespace prime_suit_map{
        
        using prime_suit_t = std::uint32_t;
        
        //                                                      2 3 4 5  6  7  8  9  T  J  Q  K  A
        static constexpr std::array<prime_suit_t, 13> Primes = {2,3,5,7,11,13,17,19,23,27,29,31,37};

        
        inline
        prime_suit_t append(prime_suit_t val, suit_id suit)noexcept{
                return val * Primes[suit];
        }
        inline
        prime_suit_t create()noexcept{ return 1; }
        template<class... Args>
        prime_suit_t create(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, args),0)...};
                return val;
        }
        template<class... Args>
        prime_suit_t create_from_cards(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, card_suit_from_id(args)),0)...};
                return val;
        }

        inline bool is_five_card_flush(prime_suit_t val)noexcept{
                switch(val){
                case 2*2*2*2*2:
                case 3*3*3*3*3:
                case 5*5*5*5*5:
                case 7*7*7*7*7:
                        return true;
                default:
                        return false;
                }
        }
        inline
        bool has_flush_unsafe(prime_suit_t val)noexcept{
                return 
                    ((val % (2*2*2*2*2))*
                     (val % (3*3*3*3*3))*
                     (val % (5*5*5*5*5))*
                     (val % (7*7*7*7*7))) == 0;
        }

        inline prime_suit_t five_card_max()noexcept{
                return 37 * 37 * 37 * 37 * 31;
        }


} // end namespace prime_suit_map
} // end namespace

#endif

#endif // PS_BASE_SUIT_HASHER_H
