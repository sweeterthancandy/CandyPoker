#ifndef PS_DRIVER_H
#define PS_DRIVER_H

#include "eval.h"

#include <ostream>
#include <string>

#include <boost/format.hpp>

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
                return ostr  << boost::format("{ wins=%d(%.2f), lose=%d(%.2f), draw=%d(%.2f) }\n")
                        % self.wins % ( self.wins / sigma )
                        % self.lose % ( self.lose / sigma )
                        % self.draw % ( self.draw / sigma );
        }
};

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

                std::set<long> known { hero_1, hero_2, villian_1, villian_2 };

                for(long c0 = 52; c0 != 0; ){
                        --c0;
                        if( known.count(c0) )
                                continue;
                
                        for(long c1 = c0; c1 != 0; ){
                                --c1;
                                if( c1 == c0 )
                                        continue;
                                if( known.count(c1) )
                                        continue;

                                for(long c2 = c1; c2 != 0; ){
                                        --c2;
                                        if( c2 == c1 )
                                                continue;
                                        if( known.count(c2) )
                                                continue;
                                
                                        for(long c3 = c2; c3 != 0; ){
                                                --c3;
                                                if( c3 == c2 )
                                                        continue;
                                                if( known.count(c3) )
                                                        continue;
                                        
                                                for(long c4 = c3; c4 != 0; ){
                                                        --c4;
                                                        if( c4 == c3 )
                                                                continue;
                                                        if( known.count(c4) )
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
                return ret;
        }
private:
        eval eval_;
        card_traits traits_;
};

} // namespace ps

#endif // PS_DRIVER_H
