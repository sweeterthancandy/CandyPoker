#ifndef PS_BASE_RANK_HASHER_H
#define PS_BASE_RANK_HASHER_H

namespace ps{

namespace rank_hasher{
        using rank_hash_t = size_t;

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
        rank_hash_t append(rank_hash_t hash, rank_id rank)noexcept{
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
                        break;
                default:
                        PS_UNREACHABLE();
                }
                return hash;
        }
        
        inline
        rank_hash_t create()noexcept{ return 0; }
        inline
        rank_hash_t create(rank_vector const& rv) noexcept{
                auto hash = create();
                for(auto id : rv )
                        hash = rank_hasher::append(hash, id);
                return hash;
        }
        template<class... Args>
        rank_hash_t create(Args... args) noexcept{
                auto hash = create();
                int _[] = {0,  (hash = append(hash, args),0)...};
                return hash;
        }
        template<class... Args>
        rank_hash_t create_from_cards(Args... args) noexcept{
                auto hash = create();
                int _[] = {0,  (hash = append(hash, card_rank_from_id(args)),0)...};
                return hash;
        }
        inline
        const rank_hash_t max()noexcept{
                return create(12,12,12,12,11,11,11);
        }
} // end namespace rank_hasher

} // ps
#endif // PS_BASE_RANK_HASHER_H
