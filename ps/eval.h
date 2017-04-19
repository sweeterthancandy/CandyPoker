#ifndef PS_EVAL_H
#define PS_EVAL_H

#include <array>

#include <boost/range/algorithm.hpp>

#include "generate.h"

#include "ps/cards.h"

namespace ps{

namespace detail{
        struct eval_impl{
                eval_impl():order_{1}{
                        m_.resize( 37 * 37 * 37 * 37 * 31 * 2 +1 );

                        std::array<int,4> suit_map{ 2,3,5,7 };
                        for( size_t i{0};i!=52;++i){
                                flush_device_[i] = suit_map[card_decl::get(i).suit().id()];
                                rank_device_[i] = card_decl::get(i).rank().id();
                        }
                }
                void next( bool f, long a, long b, long c, long d, long e){
                        std::uint32_t m = map(f,a,b,c,d,e);
                        assert( m_[m] == 0 && "not injective");
                        m_[m] = order_;
                        ++order_;
                }
                void begin(std::string const& name){
                        name_ = name;
                }
                void end(){}
                std::uint32_t eval(long a, long b, long c, long d, long e)const{
                        auto f_aux{ flush_device_[a] * 
                                    flush_device_[b] *
                                    flush_device_[c] *
                                    flush_device_[d] *
                                    flush_device_[e] };

                        bool f{false};
                        switch(f_aux){
                        case 2*2*2*2*2:
                        case 3*3*3*3*3:
                        case 5*5*5*5*5:
                        case 7*7*7*7*7:
                                f = true;
                                break;
                        }
                        std::uint32_t m = map(f,
                                              rank_device_[a],rank_device_[b],
                                              rank_device_[c],rank_device_[d],
                                              rank_device_[e]);
                        assert( m_[m] && "unmapped value");
                        return m_[m];
                }
                std::uint32_t map(bool f, long a, long b, long c, long d, long e)const{
                        //                                 2 3 4 5  6  7  8  9  T  J  Q  K  A
                        std::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                        return ( f ? 2 : 1 ) * 
                                p[a] * p[b] * 
                                p[c] * p[d] * p[e];
                }
        private:
                size_t order_;
                std::string name_;
                std::vector<std::uint32_t> m_;

                std::array<int, 52> flush_device_;
                std::array<int, 52> rank_device_;
        };
}

struct eval{
        eval(){
                generate(impl_);
        }
        std::uint32_t eval_5(std::vector<long> const& cards)const{
                assert( cards.size() == 5 && "precondition failed");
                return this->operator()(cards[0], cards[1], cards[2], cards[3], cards[4] );
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e)const{
                return impl_.eval(a,b,c,d,e);
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e, long f)const{
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
        std::uint32_t operator()(long a, long b, long c, long d, long e, long f, long g)const{
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
private:
        detail::eval_impl impl_;
};

} // namespace ps

#endif // #ifndef PS_EVAL_H
