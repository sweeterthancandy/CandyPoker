#ifndef PS_DRIVER_H
#define PS_DRIVER_H

#include "eval.h"

#include <ostream>
#include <string>

#include <boost/format.hpp>

#include "detail/void_t.h"

namespace ps{

struct an_result_t{
        size_t wins;
        size_t lose;
        size_t draw;

        friend std::ostream& operator<<(std::ostream& ostr, an_result_t const& self){
                auto sigma{ static_cast<double>(self.wins + self.lose + self.draw )};
                #if 0
                return ostr 
                        << "{ wins=" << self.wins 
                        << ", lose=" << self.lose 
                        << ", draw=" << self.draw 
                        << "}\n";
                #endif
                return ostr  << boost::format("{ wins=%d(%.2f), lose=%d(%.2f), draw=%d(%.2f) }")
                        % self.wins % ( self.wins / sigma * 100 )
                        % self.lose % ( self.lose / sigma * 100 )
                        % self.draw % ( self.draw / sigma * 100 );
        }
};


namespace detail{
        template<int N, class V, class F, class... Args>
        std::enable_if_t<N==0> visit_combinations(V v, F f, long upper, Args&&... args){
                v(std::forward<Args>(args)...);
        }
        template<int N, class V, class F, class... Args>
        std::enable_if_t<N!=0> visit_combinations(V v, F f, long upper, Args&&... args){
                for(long iter{upper+1};iter!=0;){
                        --iter;
                        if( ! f(iter) )
                                continue;
                        visit_combinations<N-1>(v, f, iter-1,std::forward<Args>(args)..., iter);
                }
        }
}

struct driver{
        std::uint32_t eval_5(std::string const& str)const{
                assert( str.size() == 10 && "precondition failed");
                std::vector<long> hand;
                for(size_t i=0;i!=10;i+=2){
                        hand.push_back(traits_.make(str[i], str[i+1]) );
                }
                return eval_.eval_5(hand);
        }

        an_result_t calc(std::string const& right, std::string const& left)const{
                an_result_t ret = {0,0,0};
                auto hero_1{ traits_.make(left.substr(0,2)) };
                auto hero_2{ traits_.make(left.substr(2,4)) };
                auto villian_1{ traits_.make(right.substr(0,2)) };
                auto villian_2{ traits_.make(right.substr(2,4)) };

                std::vector<long> known { hero_1, hero_2, villian_1, villian_2 };
                boost::sort(known);

                detail::visit_combinations<5>( [&](long a, long b, long c, long d, long e){
                        for( long i : {a,b,c,d,e} ){
                                if( boost::binary_search(known, i)){
                                        return;
                                }
                        }
                        auto h{ eval_(hero_1   , hero_2   , a,b,c,d,e)};
                        auto v{ eval_(villian_1, villian_2, a,b,c,d,e)};

                        if( h < v ) {
                        ++ret.wins;
                        } else if ( v < h ) {
                        ++ret.lose;
                        } else {
                        ++ret.draw;
                        }
                }, 
                [&](long c){
                        return ! boost::binary_search(known, c);
                },
                51);
                #if 0
                for(long c0 = 52; c0 != 0; ){
                        --c0;
                        if( boost::binary_search(known, c0))
                                continue;
                
                        for(long c1 = c0; c1 != 0; ){
                                --c1;
                                if( c1 == c0 )
                                        continue;
                                if( boost::binary_search(known, c1))
                                        continue;

                                for(long c2 = c1; c2 != 0; ){
                                        --c2;
                                        if( c2 == c1 )
                                                continue;
                                        if( boost::binary_search(known, c2))
                                                continue;
                                
                                        for(long c3 = c2; c3 != 0; ){
                                                --c3;
                                                if( c3 == c2 )
                                                        continue;
                                                if( boost::binary_search(known, c3))
                                                        continue;
                                        
                                                for(long c4 = c3; c4 != 0; ){
                                                        --c4;
                                                        if( c4 == c3 )
                                                                continue;
                                                        if( boost::binary_search(known, c4))
                                                                continue;

                                                        auto h{ eval_(hero_1   , hero_2   , c0, c1, c2, c3, c4 ) };
                                                        auto v{ eval_(villian_1, villian_2, c0, c1, c2, c3, c4 ) };

                                                        if( h < v ) {
                                                                ++ret.wins;
                                                        } else if ( v < h ) {
                                                                ++ret.lose;
                                                        } else {
                                                                ++ret.draw;
                                                        }
                                                }
                                        }
                                }
                        }
                }
                #endif
                return ret;
        }
private:
        eval eval_;
        card_traits traits_;
};

} // namespace ps

#endif // PS_DRIVER_H
