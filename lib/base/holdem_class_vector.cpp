#include "ps/base/holdem_class_vector.h"
#include "ps/base/cards.h"
#include "ps/base/algorithm.h"

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
        std::tuple<
                std::vector<int>,
                holdem_class_vector
        > holdem_class_vector::to_standard_form()const{
                std::vector<std::tuple<holdem_class_id, int> > aux;
                for(int i=0;i!=size();++i){
                        aux.emplace_back( (*this)[i], i);
                }
                boost::sort( aux, [](auto const& l, auto const& r){
                        return std::get<0>(l) < std::get<0>(r);
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
        
        std::vector<
               std::tuple< std::vector<int>, holdem_hand_vector >
        > holdem_class_vector::to_standard_form_hands()const{
                auto const n = size();

                std::map<holdem_hand_vector, std::vector<int>  > result;

                for( auto hv : get_hand_vectors()){

                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);

                        if( result.count(perm_players) == 0 ){
                                result[perm_players].resize(n*n);
                        }
                        auto& item = result.find(perm_players)->second;
                        for(int i=0;i!=n;++i){
                                ++item[i*n + perm[i]];
                        }
                }
                std::vector< std::tuple< std::vector<int>, holdem_hand_vector > > ret;
                for( auto& m : result ){
                        ret.emplace_back( std::move(m.second), std::move(m.first));
                }
                return std::move(ret);
        }

        bool holdem_class_vector::is_standard_form()const{
                for( size_t idx = 1; idx < size();++idx){
                        if( (*this)[idx-1] > (*this)[idx] )
                                return false; 
                }
                return true;
        }
} // ps
