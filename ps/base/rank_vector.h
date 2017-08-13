#ifndef PS_BASE_RANK_VECTOR_H
#define PS_BASE_RANK_VECTOR_H

#include <vector>
#include <ostream>

#include "cards.h"

namespace ps{

        struct rank_vector : std::vector<rank_id>{
                template<class... Args>
                rank_vector(Args&&... args):std::vector<rank_id>{std::forward<Args>(args)...}{}

                friend std::ostream& operator<<(std::ostream& ostr, rank_vector const& self);
        };

} // ps

#endif // PS_BASE_RANK_VECTOR_H 
