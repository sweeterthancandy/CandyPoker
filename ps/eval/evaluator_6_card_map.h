#ifndef PS_EVAL_EVALUATOR_6_CARD_MAP_H
#define PS_EVAL_EVALUATOR_6_CARD_MAP_H

#include "ps/eval/evaluator_5_card_map.h"
#include "ps/base/prime_rank_map.h"
#include "ps/base/suit_hasher.h"

namespace ps{



struct evaluator_6_card_map{

        evaluator_6_card_map(){
                cache_6_.resize( prime_rank_map::six_card_max()+1);

                for(board_combination_iterator iter(6),end;iter!=end;++iter){
                        auto a = suit_hasher::create(*iter);
                        // the 6 card map ONLY works when there is no flush
                        if( suit_hasher::has_flush_unsafe(a) )
                                continue;
                        
                        auto b = prime_rank_map::create(*iter);
                        auto const& cv = *iter;
                        auto r = e5cm_->rank(cv[0], cv[1], cv[2], cv[3], cv[4], cv[5]);
                        cache_6_[b] = r;
                }
        }
        evaluator_6_card_map(evaluator_6_card_map const&)=delete;
        evaluator_6_card_map(evaluator_6_card_map&&)=delete;
        evaluator_6_card_map& operator=(evaluator_6_card_map const&)=delete;
        evaluator_6_card_map& operator=(evaluator_6_card_map&)=delete;

        static evaluator_6_card_map* instance(){
                static auto ptr = new evaluator_6_card_map;
                return ptr;
        }


        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e)const noexcept{
                return e5cm_->rank(a,b,c,d,e);
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f)const noexcept{
                auto S =  suit_hasher::create_from_cards(a,b,c,d,e,f);

                if( suit_hasher::has_flush_unsafe(S) ){
                        return e5cm_->rank(a,b,c,d,e,f);
                }

                auto R = prime_rank_map::create_from_cards(a,b,c,d,e,f);
                return cache_6_[R];
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f, card_id g)const noexcept{
                auto S =  suit_hasher::create_from_cards(a,b,c,d,e,f,g);

                if( suit_hasher::has_flush_unsafe(S) ){
                        return e5cm_->rank(a,b,c,d,e,f,g);
                }

                // I know this is the quest way
                // TODO, figure out why this doesn't work
                #if 0
                auto R = prime_rank_map::create_from_cards(a,b,c,d,e,f,g);
                std::array<ranking_t, 7> aux = {
                        cache_6_[ prime_rank_map::remove_card(R,a) ],
                        cache_6_[ prime_rank_map::remove_card(R,b) ],
                        cache_6_[ prime_rank_map::remove_card(R,c) ],
                        cache_6_[ prime_rank_map::remove_card(R,d) ],
                        cache_6_[ prime_rank_map::remove_card(R,e) ],
                        cache_6_[ prime_rank_map::remove_card(R,f) ],
                        cache_6_[ prime_rank_map::remove_card(R,g) ],
                };
                #else
                std::array<ranking_t, 7> aux = {
                        cache_6_[ prime_rank_map::create_from_cards(  b,c,d,e,f,g) ],
                        cache_6_[ prime_rank_map::create_from_cards(a,  c,d,e,f,g) ],
                        cache_6_[ prime_rank_map::create_from_cards(a,b,  d,e,f,g) ],
                        cache_6_[ prime_rank_map::create_from_cards(a,b,c,  e,f,g) ],
                        cache_6_[ prime_rank_map::create_from_cards(a,b,c,d,  f,g) ],
                        cache_6_[ prime_rank_map::create_from_cards(a,b,c,d,e,  g) ],
                        cache_6_[ prime_rank_map::create_from_cards(a,b,c,d,e,f  ) ],
                };
                #endif
                return * std::min_element(aux.begin(), aux.end());
        }
private:
        evaluator_5_card_map* e5cm_{evaluator_5_card_map::instance()};
        std::vector<ranking_t> cache_6_;
};


} // ps 
#endif // PS_EVAL_EVALUATOR_6_CARD_MAP_H
