#ifndef PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H

#include "ps/base/holdem_class_range.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/detail/cross_product.h"

namespace ps{
        struct holdem_class_range_vector : std::vector<holdem_class_range>{
                template<class... Args>
                holdem_class_range_vector(Args&&... args)
                : std::vector<holdem_class_range>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range_vector const& self){
                        return ostr << detail::to_string(self);
                }

                void push_back(std::string const& s){
                        this->emplace_back(s);
                }

                // Return this expand, ie 
                //        {{AA,KK},{22}} => {AA,22}, {KK,22}
                std::vector<holdem_class_vector> get_cross_product()const{
                        std::vector<holdem_class_vector> ret;
                        detail::cross_product_vec([&](auto const& byclass){
                                ret.emplace_back();
                                for( auto iter : byclass ){
                                        ret.back().emplace_back(*iter);
                                }
                        }, *this);
                        return std::move(ret);
                }

        };
} // ps
#endif // PS_BASE_HOLDEM_CLASS_RANGE_VECTOR_H
