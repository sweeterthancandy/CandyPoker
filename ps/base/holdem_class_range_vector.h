#ifndef PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H

#include "ps/base/holdem_class_range.h"
#include "ps/base/holdem_hand_vector.h"
#include "ps/base/holdem_class_vector.h"

namespace ps{
        struct holdem_class_range_vector : std::vector<holdem_class_range>{
                template<class... Args>
                holdem_class_range_vector(Args&&... args)
                : std::vector<holdem_class_range>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range_vector const& self);

                void push_back(std::string const& s);

                // Return this expand, ie 
                //        {{AA,KK},{22}} => {AA,22}, {KK,22}
                std::vector<holdem_class_vector> get_cross_product()const;
                // Returns this as a vector of
                //        (matrix, standard-form-hand-vector)
                std::vector<
                       std::tuple< std::vector<int>, holdem_hand_vector >
                > to_standard_form()const;
                // Returns this as a vector of
                //        (matrix, standard-form-class-vector)
                std::vector<
                       std::tuple< std::vector<int>, holdem_class_vector >
                > to_class_standard_form()const;

        };
} // ps
#endif // PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H
