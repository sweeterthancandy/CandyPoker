#ifndef PS_BASE_CARD_VECTOR_H
#define PS_BASE_CARD_VECTOR_H

#include <vector>
#include <ostream>

#include "ps/base/cards_fwd.h"

namespace ps{


        struct card_vector : std::vector<card_id>{
                template<class... Args>
                card_vector(Args&&... args):std::vector<card_id>{std::forward<Args>(args)...}{}

                size_t mask()const{
                        size_t m{0};
                        for( auto id : *this ){
                                m |= ( static_cast<size_t>(1) << id );
                        }
                        return m;
                }
                static card_vector from_bitmask(size_t mask);

                friend std::ostream& operator<<(std::ostream& ostr, card_vector const& self);
        };

} // ps

#endif // PS_BASE_CARD_VECTOR_H 
