#ifndef PS_BASE_CARD_VECTOR_H
#define PS_BASE_CARD_VECTOR_H

#include <vector>
#include <ostream>

#include "cards.h"

namespace ps{

        namespace detail{
                struct card_caster{
                        template<class T>
                        std::string operator()(T id)const{
                                return card_decl::get(id).to_string();
                        }
                };
        } // detail

        struct card_vector : std::vector<card_id>{
                template<class... Args>
                card_vector(Args&&... args):std::vector<card_id>{std::forward<Args>(args)...}{}

                friend std::ostream& operator<<(std::ostream& ostr, card_vector const& self){
                        return ostr << detail::to_string(self, detail::card_caster{} );
                }
        };

} // ps

#endif // PS_BASE_CARD_VECTOR_H 
