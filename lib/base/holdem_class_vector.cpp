#include "ps/base/holdem_class_vector.h"
#include "ps/base/cards.h"

namespace ps{
        std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_class_decl::get(id).to_string();
                                                 } );
        }
        holdem_class_decl const& holdem_class_vector::decl_at(size_t i)const{
                return holdem_class_decl::get( 
                        this->operator[](i)
                );
        }

        std::vector< holdem_hand_vector > holdem_class_vector::get_hand_vectors()const{
                std::vector< holdem_hand_vector > stack;
                stack.emplace_back();

                for(size_t i=0; i!= this->size(); ++i){
                        decltype(stack) next_stack;
                        auto const& hand_set =  this->decl_at(i).get_hand_set() ;
                        for( size_t j=0;j!=hand_set.size(); ++j){
                                for(size_t k=0;k!=stack.size();++k){
                                        next_stack.push_back( stack[k] );
                                        next_stack.back().push_back( hand_set[j].id() );
                                        if( ! next_stack.back().disjoint() )
                                                next_stack.pop_back();
                                }
                        }
                        stack = std::move(next_stack);
                }
                return std::move(stack);
        }
} // ps
