#include "ps/eval/holdem_class_vector_cache.h"
#include "ps/support/persistent_impl.h"
#include "ps/base/algorithm.h"
#include "ps/eval/class_cache.h"

namespace{
        using namespace ps;
        struct n_player_impl : support::persistent_memory_impl_serializer<holdem_class_vector_pair_cache>{
                explicit n_player_impl(size_t N):N_{N}{}
                virtual std::string name()const override{
                        return "n_player_impl@" + boost::lexical_cast<std::string>(N_);
                }
                virtual std::shared_ptr<holdem_class_vector_pair_cache> make()const override{
                        auto ptr = std::make_shared<holdem_class_vector_pair_cache>();
                        std::vector<holdem_class_vector_cache_item> cache;
                        double total_count = 0.0;
                        size_t n = 0;
                        
                        class_cache cc;
                        std::string cache_name{".cc.bin"};
                        cc.load(cache_name);

                        holdem_class_vector_cache_item_pair* head = nullptr;

                        for(holdem_class_perm_iterator iter(N_),end;iter!=end;++iter){
                                auto const& cv = *iter;
                        
                                size_t count = 0;
                                switch(N_){
                                        case 2:
                                        {
                                                auto const& A =  holdem_class_decl::get(cv[0]).get_hand_set() ;
                                                auto const& B =  holdem_class_decl::get(cv[1]).get_hand_set() ;
                                                for( auto const& a : A ){
                                                        for( auto const& b : B ){
                                                                if( ! disjoint(a,b) )
                                                                        continue;
                                                                ++count;
                                                        }
                                                }
                                                if( count == 0 )
                                                        continue;
                                                break;
                                        }
                                        case 3:
                                        {
                                                auto const& A =  holdem_class_decl::get(cv[0]).get_hand_set() ;
                                                auto const& B =  holdem_class_decl::get(cv[1]).get_hand_set() ;
                                                auto const& C =  holdem_class_decl::get(cv[2]).get_hand_set() ;
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
                                                break;
                                        }
                                        default:
                                        {
                                                throw std::domain_error("not implemented");
                                        }
                                }

                                total_count += count;
                                
                                auto ev = cc.LookupVector(cv);

                                if( head == nullptr || head->cid != cv[0] ){
                                        ptr->emplace_back();
                                        head = &ptr->back();
                                        head->cid = cv[0];
                                }

                                head->vec.emplace_back();
                                head->vec.back().cv = *iter;
                                head->vec.back().count = count;
                                head->vec.back().ev.resize(N_);
                                for(size_t idx=0;idx!=N_;++idx){
                                        head->vec.back().ev[idx] = ev[idx];
                                }
                                ++n;
                                double factor = std::pow(169.0, 1.0 * N_);
                                if( n % 169 == 0 ) 
                                        std::cout << "n * 100.0 / factor => " << n * 100.0 / factor << "\n"; // __CandyPrint__(cxx-print-scalar,n * 100.0 / factor)
                        }
                        for(auto& group : *ptr){
                                for(auto& _ : group.vec){
                                        _.prob = _.count / total_count;
                                }
                        }
                        return ptr;
                }
                virtual void display(std::ostream& ostr)const override{
                        auto const& obj = *reinterpret_cast<holdem_class_vector_pair_cache const*>(ptr());
                        double sigma = 0.0;
                        for(auto& group : obj){
                                for(auto& _ : group.vec){
                                        std::cout << _ << "\n";
                                        sigma += _.prob;
                                }
                        }

                        std::cout << "sigma => " << sigma << "\n"; // __CandyPrint__(cxx-print-scalar,sigma)
                }
        private:
                size_t N_;
        };
} // end namespace anon

namespace ps{

support::persistent_memory_decl<holdem_class_vector_pair_cache> Memory_ThreePlayerClassVector( std::make_unique<n_player_impl>(3) );
support::persistent_memory_decl<holdem_class_vector_pair_cache> Memory_TwoPlayerClassVector( std::make_unique<n_player_impl>(2) );

} // end namespace ps
