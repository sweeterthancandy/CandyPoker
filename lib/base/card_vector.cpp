#include "ps/base/cards.h"

namespace ps{
        std::ostream& operator<<(std::ostream& ostr, card_vector const& self){
                return ostr << detail::to_string(self, [](auto id){
                        return card_decl::get(id).to_string();
                });
        }
        card_vector card_vector::from_bitmask(size_t mask){
                card_vector vec;
                for(size_t i=0;i!=52;++i){
                        if( mask & card_decl::get(i).mask() ){
                                vec.push_back(i);
                        }
                }
                return std::move(vec);
        }
} // ps
