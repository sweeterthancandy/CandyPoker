#ifndef PS_HAND_SET_H
#define PS_HAND_SET_H

#include <vector>

#include "ps/cards_fwd.h"
#include "ps/detail/print.h"

namespace ps{

        namespace detail{
                struct hand_caster{
                        template<class T>
                        std::string operator()(T id)const{
                                return holdem_hand_decl::get(id).to_string();
                        }
                };
                struct class_caster{
                        template<class T>
                        std::string operator()(T id)const{
                                return holdem_class_decl::get(id).to_string();
                        }
                };
        }

        struct holdem_hand_vector : std::vector<ps::holdem_id>{
                template<class... Args>
                holdem_hand_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {};
                friend std::ostream& operator<<(std::ostream& ostr, holdem_hand_vector const& self){
                        return ostr << detail::to_string(self, detail::hand_caster{} );
                }
        };


        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {};
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self){
                        return ostr << detail::to_string(self, detail::class_caster{} );
                }
                holdem_class_decl const& decl_at(size_t i)const{
                        return holdem_class_decl::get( 
                                this->operator[](i)
                        );
                }

                std::vector< holdem_hand_vector > get_hand_vectors()const{
                        std::vector< holdem_hand_vector > stack;
                        stack.emplace_back();

                        for(size_t i=0; i!= this->size(); ++i){
                                decltype(stack) next_stack;
                                auto const& hand_set{ this->decl_at(i).get_hand_set() };
                                for( size_t j=0;j!=hand_set.size(); ++j){
                                        for(size_t k=0;k!=stack.size();++k){
                                                next_stack.push_back( stack[k] );
                                                next_stack.back().push_back( hand_set[j].id() );
                                                if( ! card_vector_disjoint( next_stack.back() ) )
                                                        next_stack.pop_back();
                                        }
                                }
                                stack = std::move(next_stack);
                        }
                        return std::move(stack);
                }
        };
} // ps

#endif // PS_HAND_SET_H
