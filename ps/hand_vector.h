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
                holdem_hand_decl const& decl_at(size_t i)const{
                        return holdem_hand_decl::get( 
                                this->operator[](i)
                        );
                }
                friend std::ostream& operator<<(std::ostream& ostr, holdem_hand_vector const& self){
                        return ostr << detail::to_string(self, detail::hand_caster{} );
                }
                auto find_injective_permutation()const{
                        auto tmp{ permutate_for_the_better(*this) };
                        return std::make_tuple(
                                std::get<0>(tmp),
                                holdem_hand_vector(std::move(std::get<1>(tmp))));
                }
                bool disjoint()const{
                        std::set<card_id> s;
                        for( auto id : *this ){
                                auto const& decl{holdem_hand_decl::get(id)};
                                s.insert( decl.first() );
                                s.insert( decl.second() );
                        }
                        return s.size() == this->size()*2;
                }
        };

        #if 0
        struct holdem_hand_vector_permutation : holdem_hand_vector{
                using perm_type = std::vector<int>;
                template<class... Args>
                holdem_hand_vector_permutation(perm_type perm, Args&&... args)
                        : holdem_hand_vector{ std::forwards<Args>(args)... }
                        , perms_(std::move(perm))
                {}
        private:
                std::vector<int> perm_;
        };
        #endif


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
                                                if( ! next_stack.back().disjoint() )
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
