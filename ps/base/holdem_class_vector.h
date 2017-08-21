#ifndef PS_BASE_HOLDEM_CLASS_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_VECTOR_H

#include <vector>
#include <map>

#include "ps/base/cards.h"
#include "ps/base/holdem_hand_vector.h"
#include "ps/detail/print.h"

namespace ps{

        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_class_id>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self);
                holdem_class_decl const& decl_at(size_t i)const;
                std::vector< holdem_hand_vector > get_hand_vectors()const;

                template<
                        class... Args,
                        class = std::enable_if_t< ! std::is_constructible<std::string, Args...>::value  >
                >
                void push_back(Args&&... args){
                        this->std::vector<ps::holdem_class_id>::push_back(std::forward<Args...>(args)...);
                }
                void push_back(std::string const& item){
                        this->push_back( holdem_class_decl::parse(item).id() );
                }
                template<class Archive>
                void serialize(Archive& ar, unsigned int){
                        ar & (*reinterpret_cast<std::vector<ps::holdem_class_id>*>(this));
                }


                std::tuple<
                        std::vector<int>,
                        holdem_class_vector
                > to_standard_form()const;
                
                std::vector<
                       std::tuple< std::vector<int>, holdem_hand_vector >
                > to_standard_form_hands()const;

                bool is_standard_form()const;
        };
        
        struct holdem_class_iterator :
                basic_index_iterator<
                holdem_class_id,
                ordered_policy,
                holdem_class_vector
                >
        {
                using impl_t = 
                        basic_index_iterator<
                                holdem_class_id,
                                ordered_policy,
                                holdem_class_vector
                        >
                        ;
                holdem_class_iterator():impl_t{}{}
                holdem_class_iterator(size_t n):
                        impl_t(n, holdem_class_decl::max_id)
                {}
        };
} // ps

#endif // PS_BASE_HOLDEM_CLASS_VECTOR_H
