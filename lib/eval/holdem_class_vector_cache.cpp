#include "ps/eval/holdem_class_vector_cache.h"
#include "ps/support/persistent_impl.h"
#include "ps/base/algorithm.h"

namespace{
        using namespace ps;
        struct impl : support::persistent_memory_impl_serializer<holdem_class_vector_cache>{
                virtual std::string name()const{
                        return "three_player_class";
                }
                virtual std::shared_ptr<holdem_class_vector_cache> make()const override{
                        auto ptr = std::make_shared<holdem_class_vector_cache>();
                        std::vector<holdem_class_vector_cache_item> cache;
                        double total_count = 0.0;
                        size_t n = 0;
                        for(holdem_class_perm_iterator iter(3),end;iter!=end;++iter){
                                auto const& cv = *iter;
                                auto const& A =  holdem_class_decl::get(cv[0]).get_hand_set() ;
                                auto const& B =  holdem_class_decl::get(cv[1]).get_hand_set() ;
                                auto const& C =  holdem_class_decl::get(cv[2]).get_hand_set() ;
                                size_t count = 0;
                                for( auto const& a : A ){
                                        for( auto const& b : B ){
                                                for( auto const& c : C ){
                                                        if( disjoint(a,b,c) ){
                                                                ++count;
                                                        }
                                                }
                                        }
                                }
                                if( count == 0 )
                                        continue;

                                total_count += count;

                                ptr->emplace_back();
                                ptr->back().cv = *iter;
                                ptr->back().count = count;
                                ++n;
                                if( n % 10 ) std::cout << "( n / 169.0/169.0/169.0 ) => " << ( n / 169.0/169.0/169.0 ) << "\n"; // __CandyPrint__(cxx-print-scalar,( n / 169.0/169.0/169.0 ))
                        }
                        for(auto& _ : *ptr){
                                _.prob = _.count / total_count;
                        }
                        return ptr;
                }
                virtual void display(std::ostream& ostr)const override{
                        auto const& obj = *reinterpret_cast<holdem_class_vector_cache const*>(ptr());
                        double sigma = 0.0;
                        for( auto const& _ : obj){
                                std::cout << _ << "\n";
                                sigma += _.prob;
                        }
                        std::cout << "sigma => " << sigma << "\n"; // __CandyPrint__(cxx-print-scalar,sigma)
                }
        };
} // end namespace anon

namespace ps{

support::persistent_memory_decl<holdem_class_vector_cache> Memory_ThreePlayerClassVector( std::make_unique<impl>() );

} // end namespace ps
