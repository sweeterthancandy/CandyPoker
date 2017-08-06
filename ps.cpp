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
#include "ps/base/frontend.h"
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

holdem_range convert_to_range(frontend::range const& rng){
        holdem_range result;
        for( auto id : expand(rng).to_holdem_vector()){
                result.set_hand(id);
        }
        return std::move(result);
}
holdem_range parse_holdem_range(std::string const& s){
        return convert_to_range( frontend::parse(s));
}

int main(){
        std::vector<holdem_range> vec;
        #if 0
        //equity 	win 	tie 	      pots won 	pots tied	
        //Hand 0: 	29.676%  	26.07% 	03.60% 	     849118284 	117404490.00   { AKo }
        //Hand 1: 	23.784%  	20.18% 	03.60% 	     657207792 	117404490.00   { ATs+ }
        //Hand 2: 	46.541%  	46.43% 	00.11% 	    1512273672 	  3517896.00   { 99-77 }
        vec.push_back(parse_holdem_range("AKo"));
        vec.push_back(parse_holdem_range("ATs+"));
        vec.push_back(parse_holdem_range("99-77"));
        #endif
	//equity 	win 	tie 	      pots won 	pots tied	
        //Hand 0: 	70.040%  	69.80% 	00.24% 	     372923544 	  1257870.00   { 99+, AKs, AKo }
        //Hand 1: 	29.960%  	29.72% 	00.24% 	     158799564 	  1257870.00   { 55 }
        vec.push_back(parse_holdem_range(" 99+, AKs, AKo "));
        vec.push_back(parse_holdem_range("55"));
        
        auto const& ec = equity_evaluator_factory::get("cached");
        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(2);

        cross_product_vec([&](auto const& byclass){
                cross_product_vec([&](auto const& byhand){
                        holdem_hand_vector v;
                        for( auto iter : byhand ){
                                v.push_back( (*iter).hand().id() );
                        }
                        result->append(*ec.evaluate( v ));
                }, byclass);
        }, vec);
        std::cout << *result << "\n";


}
