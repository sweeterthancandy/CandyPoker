#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <codecvt>

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "ps/calculator.h"
#include "ps/frontend.h"
#include "ps/tree.h"
#include "ps/cards.h"
#include "ps/algorithm.h"
#include "ps/equity_calc_detail.h"

#include <type_traits>
#include <functional>
#include <iostream>
#include <future>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>


#include "ps/detail/print.h"
#include "ps/calculator_result.h"
#include "ps/calculator_view.h"

using namespace ps;

bool card_vector_disjoint(std::vector<ps::holdem_id> const& cards){
        std::set<card_id> s;
        for( auto id : cards ){
                s.insert( holdem_hand_decl::get(id).first() );
                s.insert( holdem_hand_decl::get(id).second() );
        }
        return s.size() == cards.size()*2;
}

struct primitive_calculation;

struct calculation_context{
        std::map<
                std::vector<ps::holdem_class_id>,
                primitive_calculation*
        > primitives;
};

struct calculation{
        virtual ~calculation()=default;
        
};

struct primitive_calculation : calculation{
};
struct permuation_view : calculation{
};
struct aggregate_calculation : calculation{
};

#if 0
struct calculation_compiler{
        std::unique_ptr<calculation>
        compile_class_evaluation(calculation_context& ctx,
                                 std::array_view<ps::holdem_class_id> const& players)
        {
                std::vector<calculation*> 
        }
};
#endif

// expands all the combinations
template<class V>
auto make_holdem_class_visit_hand_combinations(V v,
                                          support::array_view<ps::holdem_class_id> const& players){
        std::vector< std::vector< holdem_id> > stack;
        stack.emplace_back();

        for(size_t i=0; i!= players.size(); ++i){
                decltype(stack) next_stack;
                auto const& hand_set{ holdem_class_decl::get( players[i] ).get_hand_set() };
                for( size_t j=0;j!=hand_set.size(); ++j){
                        for(size_t k=0;k!=stack.size();++k){
                                next_stack.push_back( stack[k] );
                                next_stack.back().push_back( hand_set[j].id() );
                                if( ! card_vector_disjoint( next_stack.back() ) )
                                        next_stack.pop_back();
                        }
                }
                stack = std::move(next_stack);
        }
        return std::move(stack);
}

#include "ps/hand_vector.h"

int main(){
        holdem_class_vector players{ holdem_class_decl::parse("A2o"),
                holdem_class_decl::parse("88"),
                holdem_class_decl::parse("QJs") };

        PRINT( players );

        for( auto hs : players.get_hand_vectors()){
                PRINT( hs );
        }

        #if 0
        std::vector< std::vector< holdem_id> > stack;
        stack.emplace_back();

        for(size_t i=0; i!= players.size(); ++i){
                decltype(stack) next_stack;
                auto const& hand_set{ holdem_class_decl::get( players[i] ).get_hand_set() };
                for( size_t j=0;j!=hand_set.size(); ++j){
                        for(size_t k=0;k!=stack.size();++k){
                                next_stack.push_back( stack[k] );
                                next_stack.back().push_back( hand_set[j].id() );
                                if( ! card_vector_disjoint( next_stack.back() ) )
                                        next_stack.pop_back();
                        }
                }
                stack = std::move(next_stack);
        }
        
        std::vector<std::tuple<std::vector<int>, std::vector< holdem_id>  > > packages;
        std::set< std::vector< holdem_id> > world;
        for( auto& s : stack ){
                auto t{ permutate_for_the_better( s ) };
                packages.emplace_back( std::move(t));
        }
        
        for( auto& t : packages ){
                world.emplace( std::get<1>(t) );
        }
        for( auto& s : world ){
                holdem_hand_vector vec{s.begin(), s.end() };
                PRINT(vec);

                //PRINT( detail::to_string(s));
        }
        #endif
}
