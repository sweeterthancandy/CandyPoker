#include "ps/base/card_vector.h"

namespace ps{
        std::ostream& operator<<(std::ostream& ostr, card_vector const& self){
                return ostr << detail::to_string(self, [](auto id){
                        return card_decl::get(id).to_string();
                });
        }
} // ps
