#include "ps/base/holdem_hand_vector.h"

#include "ps/base/cards.h"
#include "ps/base/algorithm.h"


namespace ps{


        holdem_hand_decl const& holdem_hand_vector::decl_at(size_t i)const{
                return holdem_hand_decl::get( 
                        this->operator[](i)
                );
        }
        std::ostream& operator<<(std::ostream& ostr, holdem_hand_vector const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_hand_decl::get(id).to_string();
                                                 } );
        }
        auto holdem_hand_vector::find_injective_permutation()const{
                auto tmp =  permutate_for_the_better(*this) ;
                return std::make_tuple(
                        std::get<0>(tmp),
                        holdem_hand_vector(std::move(std::get<1>(tmp))));
        }
        bool holdem_hand_vector::disjoint()const{
                std::set<card_id> s;
                for( auto id : *this ){
                        auto const& decl = holdem_hand_decl::get(id);
                        s.insert( decl.first() );
                        s.insert( decl.second() );
                }
                return s.size() == this->size()*2;
        }

        bool holdem_hand_vector::is_standard_form()const{
                auto p =  permutate_for_the_better(*this);
                auto const& perm = std::get<0>(p);
                // TODO, need to make sure AA KK KK QQ persevers order etc
                for( int i=0;i!=perm.size();++i){
                        if( perm[i] != i )
                                return false;
                }
                return true;
        }
                
} // ps
