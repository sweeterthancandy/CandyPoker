#ifndef PS_BASE_HOLDEM_HAND_VECTOR_H
#define PS_BASE_HOLDEM_HAND_VECTOR_H

#include <vector>

#include "ps/support/index_sequence.h"
#include "ps/base/cards_fwd.h"
#include "ps/base/card_vector.h"
#include "ps/detail/print.h"

namespace ps{

        /*
                Hand vector is a vector of hands
         */
        struct holdem_hand_vector : std::vector<ps::holdem_id>{
                template<class... Args>
                holdem_hand_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                holdem_hand_decl const& decl_at(size_t i)const;
                friend std::ostream& operator<<(std::ostream& ostr, holdem_hand_vector const& self);
                std::tuple<
                       std::vector<int>,
                       holdem_hand_vector
                > to_standard_form()const;
                bool disjoint()const;
                bool is_standard_form()const;
                size_t mask()const;
                card_vector to_card_vector()const;
                template<class Archive>
                void serialize(Archive& ar, unsigned int){
                        ar & (*reinterpret_cast<std::vector<ps::holdem_id>*>(this));
                }
        };

        namespace detail{

                template<class Policy>
                struct basic_holdem_hand_deal_iterator :
                        basic_index_iterator<
                                holdem_id,
                                Policy,
                                holdem_hand_vector
                        >
                {
                        using impl_t = 
                                basic_index_iterator<
                                        holdem_id,
                                        Policy,
                                        holdem_hand_vector
                                >
                        ;
                        basic_holdem_hand_deal_iterator():impl_t{}{}
                        basic_holdem_hand_deal_iterator(size_t n):
                                impl_t(n, 52 * 51 / 2)
                        {
                                eat_non_disjoint_();
                        }
                        basic_holdem_hand_deal_iterator& operator++(){
                                impl_t::operator++();
                                eat_non_disjoint_();
                                return *this;
                        }
                private:
                        void eat_non_disjoint_(){
                                for(;!this->eos();){
                                        if( this->operator->()->disjoint() )
                                                break;
                                        impl_t::operator++();
                                }
                        }
                };
        } // detail

        // for when A < B < C
        using holdem_hand_iterator = detail::basic_holdem_hand_deal_iterator<strict_lower_triangle_policy>;
        // for when we have {A,B}, {B,A}
        using holdem_hand_deal_iterator = detail::basic_holdem_hand_deal_iterator<range_policy>;
        

} // ps

#endif // PS_BASE_HOLDEM_HAND_VECTOR_H
