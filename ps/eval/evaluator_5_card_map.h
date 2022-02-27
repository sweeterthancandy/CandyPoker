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
#ifndef PS_EVAL_EVALUATOR_5_CARD_MAP_H
#define PS_EVAL_EVALUATOR_5_CARD_MAP_H


#include <sstream>
#include <mutex>

#include <boost/range/algorithm.hpp>
#include <boost/timer/timer.hpp>
#include "ps/eval/rank_decl.h"
#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

#include "ps/base/visit_poker_rankings.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/prime_rank_map.h"


/*
        At the lowest level we have the 5,6, and 7 card evaluator. This maps 7 distinct
        cards from the desk, to number in [0,M], such that 0 is a royal flush, and
        M is 7 high.
                As an implementation detail, a 5 card evaluation isn't too complicated
        because we can just split the card into two groups, though without a flush
        and those with a flush, and then each of these can be represented as an array
        lookup.
                But 6 and 7 card evaluations are more complicated because there's too
        much memory to create a canonical 7 card array lookup, by a 7 card look up is
                        Inf { rank(v) : v is 5 cards from w },
        however this isn't too efficent
 */

namespace ps{

struct evaluator_5_card_map{
        evaluator_5_card_map(){

                struct V{
                        void begin(std::string const&){}
                        void end(){}
                        void next( bool f, long a, long b, long c, long d, long e){
                                auto m = prime_rank_map::create(
                                    static_cast<rank_id>(a),
                                    static_cast<rank_id>(b),
                                    static_cast<rank_id>(c),
                                    static_cast<rank_id>(d),
                                    static_cast<rank_id>(e));
                                if( f )
                                        self_->flush_map_[m] = order_;
                                else
                                        self_->rank_map_[m] = order_;
                                ++order_;
                        }
                        evaluator_5_card_map* self_;
                        ranking_t order_{1};
                };
                V v = {this};
                flush_map_.resize( suit_hasher::five_card_max() +1 );
                rank_map_.resize( prime_rank_map::five_card_max() + 1 );
                visit_poker_rankings(v);
        }


        static evaluator_5_card_map* instance(){
                static auto ptr = new evaluator_5_card_map;
                return ptr;
        }



        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e)const noexcept{

                auto f_aux = suit_hasher::create_from_cards(a,b,c,d,e);
                auto m     = prime_rank_map::create_from_cards(a,b,c,d,e);

                ranking_t ret;
                if( suit_hasher::is_five_card_flush(f_aux) ){
                        ret = flush_map_[m];
                } else{
                        ret = rank_map_[m];
                }
                return ret;
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f)const noexcept{
                std::array<ranking_t, 6> aux { 
                        rank(  b,c,d,e,f),
                        rank(a,  c,d,e,f),
                        rank(a,b,  d,e,f),
                        rank(a,b,c,  e,f),
                        rank(a,b,c,d,  f),
                        rank(a,b,c,d,e  )
                };
                return * std::min_element(aux.begin(), aux.end());
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f, card_id g)const noexcept{
                std::array<ranking_t, 7> aux = {
                        rank(  b,c,d,e,f,g),
                        rank(a,  c,d,e,f,g),
                        rank(a,b,  d,e,f,g),
                        rank(a,b,c,  e,f,g),
                        rank(a,b,c,d,  f,g),
                        rank(a,b,c,d,e,  g),
                        rank(a,b,c,d,e,f  )
                };
                return * std::min_element(aux.begin(), aux.end());
        }
private:
        std::vector<ranking_t> flush_map_;
        std::vector<ranking_t> rank_map_;
};

} // ps
#endif // PS_EVAL_EVALUATOR_5_CARD_MAP_H
