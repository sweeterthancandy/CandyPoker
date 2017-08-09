#ifndef PS_BASE_HOLDEM_CLASS_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_VECTOR_H

#include <vector>

#include "ps/base/cards_fwd.h"
#include "ps/base/holdem_hand_vector.h"
#include "ps/detail/print.h"

namespace ps{

        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self);
                holdem_class_decl const& decl_at(size_t i)const;
                std::vector< holdem_hand_vector > get_hand_vectors()const;
        };
} // ps

#endif // PS_BASE_HOLDEM_CLASS_VECTOR_H
