#include "ps/eval/eval.h"

#include <sstream>
#include <mutex>

#include <boost/range/algorithm.hpp>
#include <boost/timer/timer.hpp>
#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

#include "ps/base/generate.h"

namespace ps{

struct _5_card_map : evaluater{
        _5_card_map(){
                flush_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );
                rank_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );

                std::array<int,4> suit_map{ 2,3,5,7 };
                for( size_t i{0};i!=52;++i){
                        flush_device_[i] = suit_map[card_decl::get(i).suit().id()];
                        rank_device_[i] = card_decl::get(i).rank().id();
                }
                generate(*this);
        }
        void begin(std::string const&){}
        void end(){}
        void next( bool f, long a, long b, long c, long d, long e){
                auto m = map_rank(a,b,c,d,e);
                if( f )
                        flush_map_[m] = order_;
                else
                        rank_map_[m] = order_;
                ++order_;
        }
        ranking_t eval_flush(std::uint32_t m)const noexcept{
                return flush_map_[m];
        }
        ranking_t eval_flush(long a, long b, long c, long d, long e)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e]);
                return eval_flush(m);
        }


        ranking_t eval_rank(std::uint32_t m)const noexcept{
                return rank_map_[m];
        }
        ranking_t eval_rank(long a, long b, long c, long d, long e)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e]);
                return eval_rank(m);
        }
        ranking_t eval_rank(long a, long b, long c, long d, long e, long f)const noexcept{
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

        // public interface
        ranking_t rank(long a, long b, long c, long d, long e)const override{
                auto f_aux =  flush_device_[a] * flush_device_[b] * flush_device_[c] * flush_device_[d] * flush_device_[e] ;
                std::uint32_t m = map_rank( rank_device_[a],
                                            rank_device_[b], 
                                            rank_device_[c],
                                            rank_device_[d], 
                                            rank_device_[e]);
                ranking_t ret;


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
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
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
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{
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
protected:
        std::array<int, 52> flush_device_;
        std::array<int, 52> rank_device_;
private:
        size_t order_ = 1;
        std::vector<ranking_t> flush_map_;
        std::vector<ranking_t> rank_map_;
};

} // ps
