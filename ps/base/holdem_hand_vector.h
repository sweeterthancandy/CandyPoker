#ifndef PS_BASE_HOLDEM_HAND_VECTOR_H
#define PS_BASE_HOLDEM_HAND_VECTOR_H

#include <vector>

#include "ps/base/cards_fwd.h"
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
                auto find_injective_permutation()const;
                bool disjoint()const;
        };

} // ps

#endif // PS_BASE_HOLDEM_HAND_VECTOR_H
