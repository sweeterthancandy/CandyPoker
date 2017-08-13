#include "ps/base/holdem_class_range_vector.h"
#include "ps/base/algorithm.h"

namespace ps{
                
std::vector<holdem_class_vector> holdem_class_range_vector::get_cross_product()const{
        std::vector<holdem_class_vector> ret;
        detail::cross_product_vec([&](auto const& byclass){
                ret.emplace_back();
                for( auto iter : byclass ){
                        ret.back().emplace_back(*iter);
                }
        }, *this);
        return std::move(ret);
}

std::vector<
       std::tuple< std::vector<int>, holdem_hand_vector >
> holdem_class_range_vector::to_standard_form()const{

        auto const n = size();

        std::map<holdem_hand_vector, std::vector<int>  > result;

        for( auto const& cv : get_cross_product()){
                for( auto hv : cv.get_hand_vectors()){

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
        }
        std::vector< std::tuple< std::vector<int>, holdem_hand_vector > > ret;
        for( auto& m : result ){
                ret.emplace_back( std::move(m.second), std::move(m.first));
        }
        return std::move(ret);
}
std::vector<
       std::tuple< std::vector<int>, holdem_class_vector >
> holdem_class_range_vector::to_class_standard_form()const{

        auto const n = size();

        std::map<holdem_class_vector, std::vector<int>  > result;

        for( auto const& cv : get_cross_product()){

                auto stdform = cv.to_standard_form();
                auto const& perm = std::get<0>(stdform);
                auto const& perm_cv = std::get<1>(stdform);

                if( result.count(perm_cv) == 0 ){
                        result[perm_cv].resize(n*n);
                }
                auto& item = result.find(perm_cv)->second;
                for(int i=0;i!=n;++i){
                        ++item[i*n + perm[i]];
                }
        }
        std::vector< std::tuple< std::vector<int>, holdem_class_vector > > ret;
        for( auto& m : result ){
                ret.emplace_back( std::move(m.second), std::move(m.first));
        }
        return std::move(ret);
}

} // ps

