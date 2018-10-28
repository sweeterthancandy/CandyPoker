#include "ps/eval/holdem_class_vector_cache.h"
#include "ps/support/persistent_impl.h"
#include "ps/base/algorithm.h"
#include "ps/eval/class_cache.h"

namespace{
        using namespace ps;
        struct three_player_impl : support::persistent_memory_impl_serializer<holdem_class_vector_cache>{
                virtual std::string name()const{
                        return "three_player_class_2";
                }
                virtual std::shared_ptr<holdem_class_vector_cache> make()const override{
                        auto ptr = std::make_shared<holdem_class_vector_cache>();
                        std::vector<holdem_class_vector_cache_item> cache;
                        double total_count = 0.0;
                        size_t n = 0;
                        
                        class_cache cc;
                        std::string cache_name{".cc.bin"};
                        cc.load(cache_name);

                        for(holdem_class_perm_iterator iter(3),end;iter!=end;++iter){
                                auto const& cv = *iter;
                                auto const& A =  holdem_class_decl::get(cv[0]).get_hand_set() ;
                                auto const& B =  holdem_class_decl::get(cv[1]).get_hand_set() ;
                                auto const& C =  holdem_class_decl::get(cv[2]).get_hand_set() ;
                                size_t count = 0;
                                for( auto const& a : A ){
                                        for( auto const& b : B ){
                                                if( ! disjoint(a,b) )
                                                        continue;
                                                for( auto const& c : C ){
                                                        if( ! disjoint(a,b,c) )
                                                                continue;
                                                        ++count;
                                                }
                                        }
                                }
                                if( count == 0 )
                                        continue;

                                total_count += count;
                                
                                auto ev = cc.LookupVector(cv);

                                ptr->emplace_back();
                                ptr->back().cv = *iter;
                                ptr->back().count = count;
                                ptr->back().ev.resize(3);
                                ptr->back().ev[0] = ev[0];
                                ptr->back().ev[1] = ev[1];
                                ptr->back().ev[2] = ev[2];
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
        struct two_player_impl : support::persistent_memory_impl_serializer<holdem_class_vector_cache>{
                virtual std::string name()const{
                        return "two_player_class_2";
                }
                virtual std::shared_ptr<holdem_class_vector_cache> make()const override{
                        auto ptr = std::make_shared<holdem_class_vector_cache>();
                        std::vector<holdem_class_vector_cache_item> cache;
                        double total_count = 0.0;
                        size_t n = 0;
                        
                        class_cache cc;
                        std::string cache_name{".cc.bin"};
                        cc.load(cache_name);

                        for(holdem_class_perm_iterator iter(2),end;iter!=end;++iter){
                                auto const& cv = *iter;
                                auto const& A =  holdem_class_decl::get(cv[0]).get_hand_set() ;
                                auto const& B =  holdem_class_decl::get(cv[1]).get_hand_set() ;
                                size_t count = 0;
                                for( auto const& a : A ){
                                        for( auto const& b : B ){
                                                if( ! disjoint(a,b) )
                                                        continue;
                                                ++count;
                                        }
                                }
                                if( count == 0 )
                                        continue;

                                total_count += count;
                                
                                auto ev = cc.LookupVector(cv);

                                ptr->emplace_back();
                                ptr->back().cv = *iter;
                                ptr->back().count = count;
                                ptr->back().ev.resize(2);
                                ptr->back().ev[0] = ev[0];
                                ptr->back().ev[1] = ev[1];
                                ++n;
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

support::persistent_memory_decl<holdem_class_vector_cache> Memory_ThreePlayerClassVector( std::make_unique<three_player_impl>() );
support::persistent_memory_decl<holdem_class_vector_cache> Memory_TwoPlayerClassVector( std::make_unique<two_player_impl>() );

} // end namespace ps
