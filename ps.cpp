#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <map>
#include <codecvt>
#include <type_traits>
#include <functional>

#include <boost/range/algorithm.hpp>
#include "ps/detail/visit_combinations.h"

#include "ps/base/range.h"
#include "ps/base/card_vector.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/evaluator.h"
#include "ps/eval/rank_world.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"


using namespace ps;

template<class V, class Vec>
void cross_product_vec( V v, Vec& vec){

        using iter_t = decltype(vec.front().begin());

        size_t const sz = vec.size();

        std::vector<iter_t> proto_vec;
        std::vector<iter_t> iter_vec;
        std::vector<iter_t> end_vec;


        for( auto& e : vec){
                proto_vec.emplace_back( e.begin() );
                end_vec.emplace_back(e.end());
        }
        iter_vec = proto_vec;

        for(;;){
                v(iter_vec);

                size_t cursor = sz - 1;
                for(;;){
                        if( ++iter_vec[cursor] == end_vec[cursor]){
                                if( cursor == 0 )
                                        return;
                                iter_vec[cursor] = proto_vec[cursor];
                                --cursor;
                                continue;
                        }
                        break;
                }
        }
}

#if 0
int main(){
        range hero, villian;
        hero.set_class( holdem_class_decl::parse("KK"));
        hero.set_class( holdem_class_decl::parse("QQ"));
        villian.set_hand( holdem_hand_decl::parse("As2h"));
        villian.set_class( holdem_class_decl::parse("22"));

        std::vector<range> aux{ hero, villian };

        cross_product_vec([](auto const& v){
                PRINT_SEQ((v[0].class_())(v[1].class_()));
                cross_product_vec([](auto const& v){
                              PRINT_SEQ((v[0].decl())(v[1].decl()));
                }, v);
        }, aux);

}
#endif

#if 0
int main(){
        using namespace decl;
        auto const& eval = evaluator_factory::get("6_card_map");
        auto const& rm = rank_word_factory::get();
        PRINT( rm[eval.rank( _Ah, _Kh, _Qh, _Jh, _Th )] );
        PRINT( rm[eval.rank( _Ah, _Kd, _Qh, _Jh, _Th )] );
        PRINT( rm[eval.rank( _Ah, _Kd, _Qh, _Jh, _Th, _2c )] );
        PRINT( rm[eval.rank( _Ah, _Kd, _Qh, _Jh, _Th, _2c, _Ad )] );
        PRINT( rm[eval.rank( _Ah, _Ad, _Qh, _Jh, _Th )] );
}
#endif


#if 0
int main(){
        size_t count=0;
        for(board_combination_iterator iter(3, std::vector<card_id>{0,2,4,5}),end;iter!=end;++iter){
                ++count;
        }
        PRINT(count);
}
#endif


#if 0
int main(){
        using namespace decl;
        // Hand 0: 	60.228%  	59.34% 	00.89% 	       1016051 	    15248.00   { Ts8h }
        // Hand 1: 	39.772%  	38.88% 	00.89% 	        665767 	    15248.00   { 7c6c }
        std::vector<holdem_id> p{ 
                holdem_hand_decl::parse("7c6c"),
                holdem_hand_decl::parse("Ts8h")
        };
        auto const& ee = equity_evaluator_factory::get("cached");
        auto ret = ee.evaluate(p);
        std::cout << *ret << "\n";
        //std::cout << ret << "\n";
}
#endif

template<class Vec>
std::tuple<
        std::vector<size_t>,
        Vec
>
make_injective_vector_permutation(Vec const& vec){

        // sort vector
        Vec sorted(vec);
        boost::sort( sorted );

        // now match up to a perm
        std::vector<size_t> perm;
        for(size_t from=0;from!=vec.size();++from){
                for(size_t to=0;to!=vec.size();++to){
                        if( vec[from] == sorted[to] &&
                            boost::find(perm, to) == perm.end()){
                                perm.emplace_back(to);
                        }
                }
        }
        return std::make_tuple(
                std::move(perm),
                std::move(sorted)
        );
}


int main(){
        // Hand 0: 	65.771%  	65.55% 	00.22% 	      26939748 	    89034.00   { 77 }
        // Hand 1: 	34.229%  	34.01% 	00.22% 	      13977480 	    89034.00   { A5s }
        holdem_hand_vector p{ 
                holdem_class_decl::parse("77"),
                holdem_class_decl::parse("A5s")
        };
        auto const& ee = class_equity_evaluator_factory::get("principal");
        auto ret = ee.evaluate(p);
        std::cout << *ret << "\n";
        //std::cout << ret << "\n";
}
