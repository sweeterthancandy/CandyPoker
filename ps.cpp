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
#include "ps/eval/range_equity_evaluator.h"

using namespace ps;

struct computational_work{
};

struct execution_strategy{
};


int main(){
        std::vector<holdem_range> vec;
        #if 1
        //equity 	win 	tie 	      pots won 	pots tied	
        //Hand 0: 	29.676%  	26.07% 	03.60% 	     849118284 	117404490.00   { AKo }
        //Hand 1: 	23.784%  	20.18% 	03.60% 	     657207792 	117404490.00   { ATs+ }
        //Hand 2: 	46.541%  	46.43% 	00.11% 	    1512273672 	  3517896.00   { 99-77 }
        vec.push_back(parse_holdem_range("AKo"));
        vec.push_back(parse_holdem_range("ATs+"));
        vec.push_back(parse_holdem_range("99-77"));
        #else
	//equity 	win 	tie 	      pots won 	pots tied	
        //Hand 0: 	70.040%  	69.80% 	00.24% 	     372923544 	  1257870.00   { 99+, AKs, AKo }
        //Hand 1: 	29.960%  	29.72% 	00.24% 	     158799564 	  1257870.00   { 55 }
        vec.push_back(parse_holdem_range(" 99+, AKs, AKo "));
        vec.push_back(parse_holdem_range("55"));
        #endif
        
        auto const& ec = range_equity_evaluator_factory::get("principal");

        auto result = ec.evaluate( vec );
        std::cout << *result << "\n";


}
