#ifndef PS_HOLDEM_CARD_TRAITS_H
#define PS_HOLDEM_CARD_TRAITS_H

#include <array>

#include "ps/core/card_traits.h"

namespace ps{
        
        /*
                To make the code pretty I want to be able to write,

                        m.f("Ah")
                        m.f(51)
                        m.f(50, 51)
                        m.f("Ahkh")
                        m.f("Ah", )

                all these cases can be halded my a traits class, ie,

                        auto f(Args&&... args){
                                g( traits_.make(std::forward<Args>(args)...) );
                        }

         */

        struct holdem_traits{
                using hand_type = std::array<long, 2>;
                using set_type  = std::vector<long>;
        };

        struct holdem_hand_maker{
                using hand_type = holdem_traits::hand_type;
                using set_type  = holdem_traits::set_type;
                
                
                set_type make_set(std::string const& str)const{
                        assert( str.size() %2 == 0 );
                        set_type result;
                        for(size_t i{0};i!=str.size();i+=2)
                                result.emplace_back( ct_.make(str.substr(i,2)));
                        return std::move(result);
                }

                hand_type make(std::string const& str)const{
                        return 
                                { ct_.make(str.substr(0,2)),
                                  ct_.make(str.substr(2,4)) };
                }
                hand_type make(long a, long b)const{
                        return {a,b};
                }
        private:
                card_traits ct_;
        };
}


#endif // PS_HOLDEM_CARD_TRAITS_H
