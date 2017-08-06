#include "5_card_map.h"

namespace ps{

struct _6_card_map : _5_card_map{
        _6_card_map(){
                cache_6_.resize( 37*37*37*37*31*31+1);
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
        ranking const&  rank_brute(long a, long b, long c, long d, long e, long f)const{
                return reinterpret_cast<_5_card_map const*>(this)
                        ->rank(a,b,c,d,e,f);
        }
        ranking const&  rank_brute(long a, long b, long c, long d, long e, long f, long g)const{
                return reinterpret_cast<_5_card_map const*>(this)
                        ->rank(a,b,c,d,e,f,g);
        }
        ranking const& rank(long a, long b, long c, long d, long e)const override{
                auto f_aux =  flush_device_[a] * flush_device_[b] * flush_device_[c] * flush_device_[d] * flush_device_[e] ;
                std::uint32_t m = map_rank( rank_device_[a],
                                            rank_device_[b], 
                                            rank_device_[c],
                                            rank_device_[d], 
                                            rank_device_[e]);
                ranking const* ret;


                switch(f_aux){
                case 2*2*2*2*2:
                case 3*3*3*3*3:
                case 5*5*5*5*5:
                case 7*7*7*7*7:
                        ret = &eval_flush(m);
                        break;
                default:
                        ret = &eval_rank(m);
                        break;
                }
                //PRINT_SEQ((a)(b)(c)(d)(e)(ret));
                return *ret;
        }
        ranking const& rank(long a, long b, long c, long d, long e, long f)const override{
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
                assert( cache_6_[m] && "unmapped value");

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
        ranking const& operator()(long a, long b, long c, long d, long e, long f, long g)const{
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
                std::array<ranking const*, 7> aux = {
                        &cache_6_[ map_rank(      r[1], r[2], r[3], r[4], r[5], r[6]) ],
                        &cache_6_[ map_rank(r[0]      , r[2], r[3], r[4], r[5], r[6]) ],
                        &cache_6_[ map_rank(r[0], r[1]      , r[3], r[4], r[5], r[6]) ],
                        &cache_6_[ map_rank(r[0], r[1], r[2]      , r[4], r[5], r[6]) ],
                        &cache_6_[ map_rank(r[0], r[1], r[2], r[3]      , r[5], r[6]) ],
                        &cache_6_[ map_rank(r[0], r[1], r[2], r[3], r[4]      , r[6]) ],
                        &cache_6_[ map_rank(r[0], r[1], r[2], r[3], r[4], r[5]      ) ]
                };
                return ** std::min_element(aux.begin(), aux.end(), [](auto const& l,
                                                                      auto const& r){
                                           return *l < *r;
                        });
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
        std::vector<ranking> cache_6_;
};

} // ps 
