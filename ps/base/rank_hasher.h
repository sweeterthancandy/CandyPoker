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
#ifndef PS_BASE_RANK_HASHER_H
#define PS_BASE_RANK_HASHER_H

#include "ps/support/config.h"
#include "ps/base/rank_board_combination_iterator.h"


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


                  We also want to remap the highest bits into the
                  lower sections, which can be done though some 
                  rules
                
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

        /*
                Array is assumed to be at least of size rank_hasher::max()+1
                eval is assumed to evaluate n-cards, such that no flush's are
                possible
         */
        template<class Array, class Eval>
        void create_rank_hash_lookup_inplace(Array& array, Eval const& eval){
                for(rank_board_combination_iterator iter(7),end;iter!=end;++iter){
                        auto const& b = *iter;
                        auto hash = rank_hasher::create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                        auto val = eval( b[0], b[1], b[2], b[3], b[4], b[5], b[6]);

                        array[hash] = val;
                }
        }
} // end namespace rank_hasher

} // ps
#endif // PS_BASE_RANK_HASHER_H
