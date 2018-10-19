#ifndef PS_EVAL_EVALUATOR_5_CARD_MAP_H
#define PS_EVAL_EVALUATOR_5_CARD_MAP_H


#include "ps/eval/evaluator.h"

#include <sstream>
#include <mutex>

#include <boost/range/algorithm.hpp>
#include <boost/timer/timer.hpp>
#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"
#include "ps/base/suit_hasher.h"

#include "ps/base/visit_poker_rankings.h"
#include "ps/base/prime_rank_map.h"



namespace ps{

struct evaluator_5_card_map{
        evaluator_5_card_map(){

                struct V{
                        void begin(std::string const&){}
                        void end(){}
                        void next( bool f, long a, long b, long c, long d, long e){
                                auto m = prime_rank_map::create(a,b,c,d,e);
                                if( f )
                                        self_->flush_map_[m] = order_;
                                else
                                        self_->rank_map_[m] = order_;
                                ++order_;
                        }
                        evaluator_5_card_map* self_;
                        size_t order_{1};
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
        ranking_t rank(long a, long b, long c, long d, long e)const{

                auto f_aux = suit_hasher::create_from_cards(a,b,c,d,e);
                auto m     = prime_rank_map::create_from_cards(a,b,c,d,e);

                ranking_t ret;
                if( suit_hasher::has_flush_unsafe(f_aux) ){
                        ret = flush_map_[m];
                } else{
                        ret = rank_map_[m];
                }
                return ret;
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const{
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
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const{
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
