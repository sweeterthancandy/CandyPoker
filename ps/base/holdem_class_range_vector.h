#ifndef PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H

#include "ps/base/holdem_class_range.h"

namespace ps{
        struct holdem_class_range_vector : std::vector<holdem_class_range>{
                template<class... Args>
                holdem_class_range_vector(Args&&... args)
                : std::vector<holdem_class_range>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range_vector const& self){
                        return ostr << detail::to_string(self);
                }
        };
} // ps
#endif // PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H
