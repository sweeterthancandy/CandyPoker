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


namespace detail{
        template<class Iter_Type>
        struct parser{
                range parse(Iter_Type start, Iter_Type end){
                        restart();

                        begin = start;
                        iter = begin;
                        last = end;

                        if( std::string{start,end} == "100%" )
                                return percent(100);
                                
                        // eat whitespace
                        for(;iter!=last && std::isspace(*iter);++iter);


                        for(;iter != last;){

                                if( ! parse_sub_range_() )
                                        error_(std::domain_error("bad syntax for (" + std::string(begin,last) + ")"));


                                if( iter == last )
                                        break;
                                
                                // seperator
                                switch(*iter){
                                case ',':
                                case ';':
                                        ++iter;
                                        continue;
                                default:
                                        if( std::isspace(*iter)){
                                                eat_ws_();
                                                continue;
                                        }
                                }
                                error_(std::domain_error("baad syntax for (" + std::string(begin,last) + ")"));
                        }

                        return working_;
                }
                template<class E>
                void error_(E const& e){
                        BOOST_THROW_EXCEPTION(e);
                }
                bool parse_sub_range_(){
                        if( ! parse_prim_() )
                                return false;
                        if( iter != last){

                                if( *iter == '+'){
                                        ++iter;
                                        working_ += plus(stack_.back());
                                        stack_.pop_back();
                                        return true;
                                }
                                if( *iter == '-'){
                                        ++iter;
                                        if( ! parse_prim_() )
                                                return false;
                                        working_ += interval(stack_[stack_.size()-2], stack_[stack_.size()-1]);
                                        stack_.pop_back();
                                        stack_.pop_back();
                                        return true;
                                }
                        }
                        working_ += stack_.back();
                        stack_.pop_back();
                        return true;
                }
                // eat whitespace
                void eat_ws_(){
                        for(;iter!=last && std::isspace(*iter);++iter);
                }
                void restart(){
                        stack_.clear();
                        working_.~range();
                        new(&working_)range;
                }
                bool parse_prim_(){
                        eat_ws_();
                        std::smatch m;

                        //std::cout << "parse_prim_(" << std::string(iter,last) << ", [stack_.size()=" << stack_.size() << "])" << std::endl;

                        if( std::regex_search( iter, last, m, rgx_XX_suit_decl ) ){
                                switch(m[0].str()[2]){
                                case 'o': case 'O':
                                        stack_.emplace_back( offsuit{rank_decl::parse(*iter), rank_decl::parse(*(iter+1))});
                                        break;
                                case 's': case 'S':
                                        stack_.emplace_back( suited{rank_decl::parse(*iter), rank_decl::parse(*(iter+1))});
                                        break;
                                }
                                iter += m[0].length();
                                return true;
                        }
                        if( std::regex_search( iter, last, m, rgx_XX) ){
                                if( m[0].str()[0] == m[0].str()[1] ){
                                        stack_.emplace_back( pocket_pair{rank_decl::parse(*iter)});
                                } else{
                                        stack_.emplace_back( any_suit{rank_decl::parse(*iter), rank_decl::parse(*(iter+1))});
                                }
                                iter += m[0].length();
                                return true;
                        }
                        // AX+, Ax
                        if( std::regex_search( iter, last, m, rgx_X_) ){
                                stack_.emplace_back( any_suit{rank_decl::parse(*iter), rank_decl::parse('2')});;
                                iter += m[0].length();
                                return true;
                        }
                        return false;
                }
        private:
                Iter_Type iter;
                Iter_Type begin;
                Iter_Type last;
                std::vector<primitive_t> stack_;
                range working_;

                
                std::regex rgx_XX_suit_decl{"^[AaKkJjQqTt[:digit:]]{2}[oOsS]"};
                std::regex rgx_XX{"^[AaKkJjQqTt[:digit:]]{2}"};
                std::regex rgx_X_{"^[AaKkJjQqTt[:digit:]][xX]"};
        };
}
        
range parse(std::string const& str){
        static detail::parser<decltype(str.begin())> p;
        return p.parse(str.begin(), str.end());
}

int main(){
        // Hand 0: 	65.771%  	65.55% 	00.22% 	      26939748 	    89034.00   { 77 }
        // Hand 1: 	34.229%  	34.01% 	00.22% 	      13977480 	    89034.00   { A5s }
        holdem_hand_vector p{ 
                holdem_class_decl::parse("77"),
                holdem_class_decl::parse("A5s")
        };
        auto const& ee = class_equity_evaluator_factory::get("principal");
        auto ret = ee.evaluate_class(p);
        std::cout << *ret << "\n";
        //std::cout << ret << "\n";
}
