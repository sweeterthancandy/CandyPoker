#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <map>
#include <codecvt>
#include <type_traits>
#include <functional>

#include "ps/base/range.h"
#include "ps/eval/evaluator.h"
#include "ps/eval/rank_world.h"

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
