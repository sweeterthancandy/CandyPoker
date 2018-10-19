#ifndef PS_EVAL_EVALUATOR_6_CARD_MAP_H
#define PS_EVAL_EVALUATOR_6_CARD_MAP_H

#include "ps/eval/evaluator_5_card_map.h"

namespace ps{

struct evaluator_6_card_map : evaluator_5_card_map{


        evaluator_6_card_map(){
                cache_6_.resize( 37*37*37*37*31*31+1);
                std::array<int,4> suit_map = { 2,3,5,7 };
                for( size_t i{0};i!=52;++i){
                        flush_device_[i] = suit_map[card_decl::get(i).suit().id()];
                        rank_device_[i] = card_decl::get(i).rank().id();
                }
                detail::visit_combinations<6>(
                        [this](long a, long b, long c, long d, long e, long f){
                                auto m =  map_rank(rank_device_[a],rank_device_[b],rank_device_[c],
                                                  rank_device_[d],rank_device_[e],rank_device_[f]);
                                auto f_aux = flush_device_[a] * 
                                            flush_device_[b] * 
                                            flush_device_[c] * 
                                            flush_device_[d] * 
                                            flush_device_[e] * 
                                            flush_device_[f];
                                if( (f_aux % (2*2*2*2*2)) == 0 ||
                                    (f_aux % (3*3*3*3*3)) == 0 ||
                                    (f_aux % (5*5*5*5*5)) == 0 ||
                                    (f_aux % (7*7*7*7*7)) == 0 )
                                {
                                        return;
                                }
                                auto ret =  this->rank_brute(a,b,c,d,e,f) ;
                                //PRINT_SEQ((a)(b)(c)(d)(e)(f)(m)(cache_6_[m]));
                                cache_6_[m] = ret;
                                //PRINT_SEQ((a)(b)(c)(d)(e)(f)(m)(cache_6_[m]));
                }, detail::true_, 51);
        }
        evaluator_6_card_map(evaluator_6_card_map const&)=delete;
        evaluator_6_card_map(evaluator_6_card_map&&)=delete;
        evaluator_6_card_map& operator=(evaluator_6_card_map const&)=delete;
        evaluator_6_card_map& operator=(evaluator_6_card_map&)=delete;

        static evaluator_6_card_map* instance(){
                static auto ptr = new evaluator_6_card_map;
                return ptr;
        }


        // some sugar
        ranking_t  rank_brute(long a, long b, long c, long d, long e, long f)const{
                return evaluator_5_card_map::rank(a,b,c,d,e,f);
        }
        ranking_t  rank_brute(long a, long b, long c, long d, long e, long f, long g)const{
                return evaluator_5_card_map::rank(a,b,c,d,e,f,g);
        }

        // only override rank or 6,7
        ranking_t rank(long a, long b, long c, long d, long e, long f)const{
                //return rank_brute(a,b,c,d,e,f);
                auto f_aux =  flush_device_[a] * flush_device_[b] * flush_device_[c] * flush_device_[d] * flush_device_[e] * flush_device_[f] ;

                if( (f_aux % (2*2*2*2*2)) == 0 ||
                    (f_aux % (3*3*3*3*3)) == 0 ||
                    (f_aux % (5*5*5*5*5)) == 0 ||
                    (f_aux % (7*7*7*7*7)) == 0 )
                {
                        return rank_brute(a,b,c,d,e,f);
                }
                auto m = map_rank( rank_device_[a],rank_device_[b], rank_device_[c],rank_device_[d], rank_device_[e], rank_device_[f]);
                //assert( cache_6_[m] && "unmapped value");

                #if 0
                bool aux{ cache_6_[m] == rank_brute(a,b,c,d,e,f)};
                std::stringstream ss;
                ss << ( aux ? "correct " : 
                              "bad     " );
                ss << card_decl::get(a);
                ss << card_decl::get(b);
                ss << card_decl::get(c);
                ss << card_decl::get(d);
                ss << card_decl::get(e);
                ss << card_decl::get(f);
                PRINT(ss.str());
                #endif

                return cache_6_[m];
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const{
                //return rank_brute(a,b,c,d,e,f,g);
                auto f_aux = flush_device_[a] * flush_device_[b] * flush_device_[c] * 
                            flush_device_[d] * flush_device_[e] * flush_device_[f] * 
                            flush_device_[g];

                if( (f_aux % (2*2*2*2*2)) == 0 ||
                    (f_aux % (3*3*3*3*3)) == 0 ||
                    (f_aux % (5*5*5*5*5)) == 0 ||
                    (f_aux % (7*7*7*7*7)) == 0 )
                {
                        return rank_brute(a,b,c,d,e,f,g);
                }
                std::array<int, 7> r = {
                        rank_device_[a], 
                        rank_device_[b],
                        rank_device_[c],
                        rank_device_[d], 
                        rank_device_[e],
                        rank_device_[f],
                        rank_device_[g]
                };
                std::array<ranking_t, 7> aux = {
                        cache_6_[ map_rank(      r[1], r[2], r[3], r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0]      , r[2], r[3], r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1]      , r[3], r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2]      , r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2], r[3]      , r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2], r[3], r[4]      , r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2], r[3], r[4], r[5]      ) ]
                };
                return * std::min_element(aux.begin(), aux.end());
#if 0

                if( aux.front() != rank_brute(a,b,c,d,e,f,g)){
                        std::stringstream ss;
                        ss << card_decl::get(a);
                        ss << card_decl::get(b);
                        ss << card_decl::get(c);
                        ss << card_decl::get(d);
                        ss << card_decl::get(e);
                        ss << card_decl::get(f);
                        ss << card_decl::get(g);
                        PRINT(ss.str());
                }
#endif
        }
private:
        std::vector<ranking_t> cache_6_;
        std::array<int, 52> flush_device_;
        std::array<int, 52> rank_device_;
};

} // ps 
#endif // PS_EVAL_EVALUATOR_6_CARD_MAP_H
