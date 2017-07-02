#include "ps/eval.h"

#include <sstream>
#include <mutex>

#include <boost/range/algorithm.hpp>
#include <boost/timer/timer.hpp>
#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

#include "ps/generate.h"

namespace ps{

struct detail_eval_impl{
        detail_eval_impl():order_{1}{
                flush_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );
                rank_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );

                std::array<int,4> suit_map{ 2,3,5,7 };
                for( size_t i{0};i!=52;++i){
                        flush_device_[i] = suit_map[card_decl::get(i).suit().id()];
                        rank_device_[i] = card_decl::get(i).rank().id();
                }
        }
        void next( bool f, long a, long b, long c, long d, long e){
                auto m = map_rank(a,b,c,d,e);
                if( f )
                        flush_map_[m] = order_;
                else
                        rank_map_[m] = order_;
                ++order_;
        }
        void begin(std::string const& name){
                name_ = name;
        }
        void end(){}

        auto eval_flush(std::uint32_t m)const noexcept{
                return flush_map_[m];
        }
        auto eval_flush(long a, long b, long c, long d, long e)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e]);
                return eval_flush(m);
        }


        auto eval_rank(std::uint32_t m)const noexcept{
                return rank_map_[m];
        }
        auto eval_rank(long a, long b, long c, long d, long e)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e]);
                return eval_rank(m);
        }
        auto eval_rank(long a, long b, long c, long d, long e, long f)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e],rank_device_[f]);
                return eval_rank(m);
        }

        std::uint32_t map_rank(long a, long b, long c, long d, long e)const{
                //                                 2 3 4 5  6  7  8  9  T  J  Q  K  A
                static std::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                return p[a] * p[b] * p[c] * p[d] * p[e];
        }
        std::uint32_t map_rank(long a, long b, long c, long d, long e, long f)const{
                //                                 2 3 4 5  6  7  8  9  T  J  Q  K  A
                static std::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                return p[a] * p[b] * p[c] * p[d] * p[e] * p[f];
        }
protected:
        std::array<int, 52> flush_device_;
        std::array<int, 52> rank_device_;
private:
        size_t order_;
        std::string name_;
        std::vector<std::uint32_t> flush_map_;
        std::vector<std::uint32_t> rank_map_;

};

struct detail_eval: detail_eval_impl{
        detail_eval(){
                cache_6_.resize( 37*37*37*37*31*31+1);
        }
        void init(){
                generate(*this);
                detail::visit_combinations<6>(
                        [this](long a, long b, long c, long d, long e, long f){
                                auto m{  map_rank(rank_device_[a],rank_device_[b],rank_device_[c],
                                                  rank_device_[d],rank_device_[e],rank_device_[f])  };
                                auto f_aux{ flush_device_[a] * 
                                            flush_device_[b] * 
                                            flush_device_[c] * 
                                            flush_device_[d] * 
                                            flush_device_[e] * 
                                            flush_device_[f] };
                                if( (f_aux % (2*2*2*2*2)) == 0 ||
                                    (f_aux % (3*3*3*3*3)) == 0 ||
                                    (f_aux % (5*5*5*5*5)) == 0 ||
                                    (f_aux % (7*7*7*7*7)) == 0 )
                                {
                                        return;
                                }
                                auto ret{ this->eval_brute(a,b,c,d,e,f) };
                                //PRINT_SEQ((a)(b)(c)(d)(e)(f)(m)(cache_6_[m]));
                                cache_6_[m] = ret;
                                //PRINT_SEQ((a)(b)(c)(d)(e)(f)(m)(cache_6_[m]));
                }, detail::true_, 51);
        }
        ~detail_eval(){
        }

        std::uint32_t operator()(long a, long b, long c, long d, long e)const{
                auto f_aux{ flush_device_[a] * flush_device_[b] * flush_device_[c] * flush_device_[d] * flush_device_[e] };
                std::uint32_t m = map_rank( rank_device_[a],
                                            rank_device_[b], 
                                            rank_device_[c],
                                            rank_device_[d], 
                                            rank_device_[e]);
                std::uint32_t ret;


                switch(f_aux){
                case 2*2*2*2*2:
                case 3*3*3*3*3:
                case 5*5*5*5*5:
                case 7*7*7*7*7:
                        ret = eval_flush(m);
                        break;
                default:
                        ret = eval_rank(m);
                        break;
                }
                //PRINT_SEQ((a)(b)(c)(d)(e)(ret));
                return ret;
        }
        std::uint32_t eval_brute(long a, long b, long c, long d, long e, long f)const{
                std::array<std::uint32_t, 6> aux { 
                        (*this)(  b,c,d,e,f),
                        (*this)(a,  c,d,e,f),
                        (*this)(a,b,  d,e,f),
                        (*this)(a,b,c,  e,f),
                        (*this)(a,b,c,d,  f),
                        (*this)(a,b,c,d,e  )
                };
                boost::sort(aux);
                return aux.front();
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e, long f)const{
                //return eval_brute(a,b,c,d,e,f);
                auto f_aux{ flush_device_[a] * flush_device_[b] * flush_device_[c] * flush_device_[d] * flush_device_[e] * flush_device_[f] };

                if( (f_aux % (2*2*2*2*2)) == 0 ||
                    (f_aux % (3*3*3*3*3)) == 0 ||
                    (f_aux % (5*5*5*5*5)) == 0 ||
                    (f_aux % (7*7*7*7*7)) == 0 )
                {
                        return eval_brute(a,b,c,d,e,f);
                }
                auto m = map_rank( rank_device_[a],rank_device_[b], rank_device_[c],rank_device_[d], rank_device_[e], rank_device_[f]);
                assert( cache_6_[m] && "unmapped value");

                #if 0
                bool aux{ cache_6_[m] == eval_brute(a,b,c,d,e,f)};
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
        std::uint32_t eval_brute(long a, long b, long c, long d, long e, long f, long g)const{
                std::array<std::uint32_t, 7> aux = {
                        (*this)(  b,c,d,e,f,g),
                        (*this)(a,  c,d,e,f,g),
                        (*this)(a,b,  d,e,f,g),
                        (*this)(a,b,c,  e,f,g),
                        (*this)(a,b,c,d,  f,g),
                        (*this)(a,b,c,d,e,  g),
                        (*this)(a,b,c,d,e,f  )
                };
                boost::sort(aux);
                return aux.front();
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e, long f, long g)const{
                //return eval_brute(a,b,c,d,e,f,g);
                auto f_aux{ flush_device_[a] * flush_device_[b] * flush_device_[c] * 
                            flush_device_[d] * flush_device_[e] * flush_device_[f] * 
                            flush_device_[g] };

                if( (f_aux % (2*2*2*2*2)) == 0 ||
                    (f_aux % (3*3*3*3*3)) == 0 ||
                    (f_aux % (5*5*5*5*5)) == 0 ||
                    (f_aux % (7*7*7*7*7)) == 0 )
                {
                        return eval_brute(a,b,c,d,e,f,g);
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
                std::array<std::uint32_t, 7> aux = {
                        cache_6_[ map_rank(      r[1], r[2], r[3], r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0]      , r[2], r[3], r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1]      , r[3], r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2]      , r[4], r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2], r[3]      , r[5], r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2], r[3], r[4]      , r[6]) ],
                        cache_6_[ map_rank(r[0], r[1], r[2], r[3], r[4], r[5]      ) ]
                };
                boost::sort(aux);
#if 0

                if( aux.front() != eval_brute(a,b,c,d,e,f,g)){
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
                return aux.front();
        }
private:
        std::vector<std::uint32_t> cache_6_;
};

/*
 * by far easiest option
 */
namespace{
        #if 0
        detail_eval* impl = new detail_eval;
        int init_ = ( impl->init(), 0 );
        inline detail_eval* get_impl(){
                return impl;
        }
        #else
        detail_eval* get_impl(){
                static detail_eval* impl = 0;
                static std::mutex mtx;
                if( impl == 0 ){
                        std::lock_guard<std::mutex> lock(mtx);
                        if( impl == 0 ){
                                auto tmp{new detail_eval};
                                tmp->init();
                                impl = tmp;
                        }
                }
                return impl;
        }
        #endif

}

eval::eval(){
}
std::uint32_t eval::eval_5(std::vector<long> const& cards)const{
        assert( cards.size() == 5 && "precondition failed");
        return this->operator()(cards[0], cards[1], cards[2], cards[3], cards[4] );
}
std::uint32_t eval::operator()(long a, long b, long c, long d, long e)const{
        return get_impl()->operator()(a,b,c,d,e);
}
std::uint32_t eval::operator()(long a, long b, long c, long d, long e, long f)const{
        return get_impl()->operator()(a,b,c,d,e, f);
}
std::uint32_t eval::operator()(long a, long b, long c, long d, long e, long f, long g)const{
        return get_impl()->operator()(a,b,c,d,e,f,g);
}



} // namespace ps

