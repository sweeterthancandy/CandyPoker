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
        
        
        struct holdem_int_traits{
                using card_type = card_traits::card_type;
                using hand_type = long;
                using set_type  = std::vector<long>;

                card_type get(hand_type hand, size_t i)const{
                        assert( i < 2 && "precondition failed");
                        card_type c{-1};
                        switch(i){
                        case 0: c =  hand / 52; break;
                        case 1: c =  hand % 52; break;
                        }
                        assert( c < 52 && "bad mapping");
                        return c;
                }
                std::string to_string(hand_type hand)const{
                        assert( hand <= 52 * 51 + 50 && "bad hand");
                        return ct_.to_string(get(hand,0)) +
                               ct_.to_string(get(hand,1));
                }
        private:
                card_traits ct_;
        };

        struct holdem_int_hand_maker{
                using card_type = card_traits::card_type;
                using hand_type = holdem_int_traits::hand_type;
                using set_type  = holdem_int_traits::set_type;
                
                
                #if 0
                set_type make_set(std::string const& str)const{
                        assert( str.size() % 2 == 0 );
                        set_type result;
                        for(size_t i{0};i!=str.size();i+=2)
                                result.emplace_back( ct_.make(str.substr(i,2)));
                        return std::move(result);
                }
                #endif

                hand_type make(std::string const& str)const{
                        return this->make(ct_.make(str.substr(0,2)),
                                          ct_.make(str.substr(2,2)));
                }
                hand_type make(long a, long b)const{
                        assert( a < 52 && "precondtion failed");
                        assert( b < 52 && "precondtion failed");
                        #if 0
                        PRINT_SEQ((a)(b));
                        PRINT_SEQ((ct_.to_string(a))(ct_.to_string(b)));
                        #endif
                        if( a < b )
                                std::swap(a, b);
                        return a * 52 + b;
                }
        private:
                card_traits ct_;
        };
}


#endif // PS_HOLDEM_CARD_TRAITS_H
