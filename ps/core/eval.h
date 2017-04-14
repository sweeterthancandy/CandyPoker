#ifndef PS_EVAL_H
#define PS_EVAL_H

#include <boost/range/algorithm.hpp>

#include "card_traits.h"
#include "generate.h"

namespace ps{

namespace detail{
        struct eval_impl{
                eval_impl():order_{1}{
                        m_.resize( 37 * 37 * 37 * 37 * 31 * 2 +1 );
                }
                void next( bool f, long a, long b, long c, long d, long e){
                        std::uint32_t m = map(f,a,b,c,d,e);
                        #if 0
                        std::cout << ( f ? "f " : "  " )
                                << traits_.rank_to_string(a)
                                << traits_.rank_to_string(b) 
                                << traits_.rank_to_string(c) 
                                << traits_.rank_to_string(d) 
                                << traits_.rank_to_string(e)
                                << "  ~  " << m
                                << "  => " << order_
                                << std::endl;
                                #endif
                        assert( m_[m] == 0 && "not injective");
                        m_[m] = order_;
                        ++order_;
                }
                void begin(std::string const& name){
                        name_ = name;
                }
                void end(){}
                std::uint32_t eval(long a, long b, long c, long d, long e)const{
                        bool f{traits_.suit(a) == traits_.suit(b) &&
                            traits_.suit(a) == traits_.suit(c) &&
                            traits_.suit(a) == traits_.suit(d) &&
                            traits_.suit(a) == traits_.suit(e)};
                        std::uint32_t m = map(f,
                                              traits_.rank(a),traits_.rank(b),
                                              traits_.rank(c),traits_.rank(d),
                                              traits_.rank(e));
                        //PRINT(m);
                        if( m_[m] == 0 ){
                                std::cout << ( f ? "f " : "  " )
                                        << traits_.to_string(a)
                                        << traits_.to_string(b) 
                                        << traits_.to_string(c) 
                                        << traits_.to_string(d) 
                                        << traits_.to_string(e)
                                        << "  ~  " << m
                                        << "\n";
                        }

                        assert( m_[m] && "unmapped value");
                        return m_[m];
                }
                std::uint32_t map(bool f, long a, long b, long c, long d, long e)const{
                        //                                   2 3 4 5  6  7  8  9  T  J  Q  K  A
                        std::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                        return ( f ? 2 : 1 ) * 
                                p[a] * p[b] * 
                                p[c] * p[d] * p[e];
                }
        private:
                size_t order_;
                std::string name_;
                std::vector<std::uint32_t> m_;
                card_traits traits_;
        };
}

struct eval{
        eval(){
                generate<card_traits>(impl_);
        }
        std::uint32_t eval_5(std::vector<long> const& cards)const{
                assert( cards.size() == 5 && "precondition failed");
                return this->operator()(cards[0], cards[1], cards[2], cards[3], cards[4] );
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e)const{
                #if 0
                std::cout
                        << traits_.rank_to_string(a)
                        << traits_.rank_to_string(b) 
                        << traits_.rank_to_string(c) 
                        << traits_.rank_to_string(d) 
                        << traits_.rank_to_string(e) << "\n";
                #endif
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
        card_traits traits_;
};

} // namespace ps

#endif // #ifndef PS_EVAL_H
