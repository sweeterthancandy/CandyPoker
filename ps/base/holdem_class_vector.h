#ifndef PS_BASE_HOLDEM_CLASS_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_VECTOR_H

#include <vector>

#include "ps/base/cards.h"
#include "ps/base/holdem_hand_vector.h"
#include "ps/detail/print.h"

#include <boost/range/algorithm.hpp>

namespace ps{

        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
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
                > to_standard_form()const{
                        std::vector<std::tuple<holdem_class_id, int> > aux;
                        for(int i=0;i!=size();++i){
                                aux.emplace_back( (*this)[i], i);
                        }
                        boost::sort( aux, [](auto const& l, auto const& r){
                                return std::get<0>(l) < std::get<1>(r);
                        });
                        std::vector<int> perm;
                        holdem_class_vector vec;
                        for( auto const& t : aux){
                                vec.push_back(std::get<0>(t));
                                perm.push_back( std::get<1>(t));
                        }
                        return std::make_tuple(
                                std::move(perm),
                                std::move(vec)
                        );
                }

                bool is_standard_form()const{
                        for( size_t idx = 1; idx < size();++idx){
                                if( (*this)[idx-1] > (*this)[idx] )
                                        return false; 
                        }
                        return true;
                }
        };
} // ps

#endif // PS_BASE_HOLDEM_CLASS_VECTOR_H
