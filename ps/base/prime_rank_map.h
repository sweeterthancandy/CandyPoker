#ifndef PS_BASE_PRIME_RANK_MAP_H
#define PS_BASE_PRIME_RANK_MAP_H

namespace ps{
namespace prime_rank_map{
        
        //                                                       2 3 4 5  6  7  8  9  T  J  Q  K  A
        static constexpr std::array<std::uint32_t, 13> Primes = {2,3,5,7,11,13,17,19,23,27,29,31,37};

        using prime_rank_t = std::uint32_t;

        inline
        prime_rank_t append(prime_rank_t val, rank_id rank)noexcept{
                return val * Primes[rank];
        }

        inline
        prime_rank_t create()noexcept{ return 1; }
        inline
        prime_rank_t create(rank_vector const& rv) noexcept{
                auto val = create();
                for(auto id : rv )
                        val = append(val, id);
                return val;
        }
        template<class... Args>
        prime_rank_t create(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, args),0)...};
                return val;
        }
        template<class... Args>
        prime_rank_t create_from_cards(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, card_rank_from_id(args)),0)...};
                return val;
        }
        inline
        const prime_rank_t max()noexcept{
                return create(12,12,12,12,11,11,11);
        }
        
        inline prime_rank_t five_card_max()noexcept{
                return 37*37*37*37*31;
        }
        inline prime_rank_t six_card_max()noexcept{
                return 37*37*37*37*31*31;
        }

} // end namespace prime_rank_map
} // end namespace ps

#endif // PS_BASE_PRIME_RANK_MAP_H
