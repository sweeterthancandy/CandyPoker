#include "ps/base/rank_vector.h"

namespace ps{
        std::ostream& operator<<(std::ostream& ostr, rank_vector const& self){
                return ostr << detail::to_string(self, [](auto id){
                                                 return rank_decl::get(id).to_string();
                });
        }
} // ps
