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
#include "ps/hand_vector.h"
#include "ps/support/push_pull.h"

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

struct calculation{
        using result_type = dyn_result_type;
        using view_type = detailed_view_type;
        using observer_type = dyn_observer_type;

        virtual ~calculation()=default;
        virtual view_type eval()=0;
};


struct permuation_view : calculation{
        using perm_type = std::vector<int>;
        permuation_view(perm_type perm, 
                        std::shared_future<result_type> fut)
                :perm_(std::move(perm))
                ,fut_(std::move(fut))
        {}
        view_type eval()override{
                auto const& result =  fut_.get() ;
                auto ret{ view_type{
                        result.n(), 
                        result.sigma(), 
                        support::array_view<size_t>{ result.data(), result.n() * result.n()},
                        perm_ } };
                return ret;
        }
private:
        perm_type perm_;
        std::shared_future<result_type> fut_;
};

struct aggregate_calculation : calculation {
        view_type eval()override{
                for( auto ptr : vec_ ){
                        agg.append(ptr->eval());
                }
                return agg.make_view();
        }
        void push_back(std::shared_ptr<calculation> ptr){
                vec_.push_back(std::move(ptr));
        }
private:
        std::vector<std::shared_ptr<calculation>> vec_;
        aggregator agg;
};


struct calculation_context{
        using result_type = dyn_result_type;
        using view_type = detailed_view_type;
        using observer_type = dyn_observer_type;

        std::vector<
                std::packaged_task<result_type(equity_calc_detail*)>
        > work;

        std::map<
                holdem_hand_vector,
                std::shared_future<result_type>
        > primitives;

        auto get_primitive_handle( holdem_hand_vector const& players ){
                auto iter = primitives.find( players );
                if( iter != primitives.end())
                        return iter->second;
                work.emplace_back( [p=players](equity_calc_detail* ec){
                        PRINT(p);
                        calculation::observer_type observer( p.size() );
                        ec->visit_boards(observer, p);
                        auto ret =  observer.make() ;
                        return ret;
                });
                primitives.emplace( players, work.back().get_future() );
                return this->get_primitive_handle(players);
        }
        void eval( equity_calc_detail* ec){
                #if 0
                for( auto& item : work){
                        item(ec);
                        PRINT("Done");
                }
                #endif
                #if 1
                support::push_pull<
                        std::function<void()>
                > pp(-1);
                PRINT( work.size() );
                pp.begin_worker();
                std::vector<std::thread> tg;
                std::atomic_int done{0};
                for(size_t i=0;i!=std::thread::hardware_concurrency();++i){
                //for(size_t i=0;i!=1;++i){
                        tg.emplace_back( [&](){
                                for(;;){
                                        auto w =  pp.pull() ;
                                        if( ! w )
                                                break;
                                        w.get()();
                                        PRINT(++done);
                                }
                        });
                }
                for( auto& item : work ){
                        pp.push( [i=&item,ec]()mutable{ (*i)(ec); });
                }
                pp.end_worker();
                for(auto& t : tg ){
                        t.join();
                }
                #endif
                #if 0
                std::vector<std::future<void> > ss;
                for( auto & item : work ){
                        ss.emplace_back(
                                std::async(
                                        std::launch::async,
                                        [i=&item,ec]()mutable{ (*i)(ec); }
                                )
                        );
                }
                for( auto& _ : ss )
                        _.get();
                #endif
        }
};

struct calculation_builder{
        static auto make( calculation_context& ctx, holdem_hand_vector const& players ){
                auto t =  players.find_injective_permutation() ;
                auto prim =  ctx.get_primitive_handle( std::get<1>(t) ) ;
                auto const& perm =  std::get<0>(t) ;
                std::vector<int> rperm;
                for(int i=0;i!= perm.size();++i)
                        rperm.push_back( perm[i] );
                auto view =  std::make_shared<permuation_view>(rperm, prim) ;
                return view;
        }
        static auto make( calculation_context& ctx, holdem_class_vector const& players ){
                auto agg = std::make_unique<aggregate_calculation>();
                for( auto hs : players.get_hand_vectors()){
                        agg->push_back( make( ctx, hs ) );
                }
                return std::shared_ptr<calculation>(agg.release());
        }
};



int main(){
        std::vector<frontend::range> players;
        players.push_back( frontend::parse("AKs") );
        players.push_back( frontend::parse("KQs") );
        players.push_back( frontend::parse("QJs") );
        players.push_back( frontend::parse("JTs") );
        players.push_back( frontend::parse("T9s") );

        players.push_back( frontend::parse("98s") );
        players.push_back( frontend::parse("87s") );
        players.push_back( frontend::parse("76s") );
        players.push_back( frontend::parse("65s") );
        #if 0
        players.push_back( frontend::parse("54s") );
        #endif
        #if 0
        players.push_back( frontend::parse("AKo") );
        players.push_back( frontend::parse("KQo") );
        players.push_back( frontend::parse("Q6s-Q4s") );
        players.push_back( frontend::parse("ATo+") );
        #endif
        //players.push_back( frontend::parse("TT-77") );

        tree_range root{ players };

        auto agg = std::make_unique<aggregate_calculation>();
        calculation_context ctx;
        for( auto const& c : root.children ){

                for( auto const& d : c.children ){
                        //auto ret =  calc.calculate_hand_equity( d.players ) ;
                        //agg.append(ret);
                        holdem_hand_vector aux{ d.players };
                        std::shared_ptr<calculation> item = calculation_builder::make(ctx, aux );
                        agg->push_back(item);

                }

        }
        equity_calc_detail ec;
        boost::timer::auto_cpu_timer at;
        ctx.eval(&ec);
        PRINT( agg->eval() );


        #if 0
        holdem_class_vector players{ holdem_class_decl::parse("AKs"),
                                     holdem_class_decl::parse("KQs"),
                                     holdem_class_decl::parse("Q6s"),
                                     holdem_class_decl::parse("87s")
        };
                                     #endif

        #if 0
        PRINT( players );


        calculation_context ctx;
        
        auto root = calculation_builder::make(ctx, players );
        equity_calc_detail ec;
        boost::timer::auto_cpu_timer at;
        ctx.eval(&ec);

        PRINT( root->eval() );
        #endif
                                         
        #if 0
        for( auto hs : players.get_hand_vectors()){
                auto t =  hs.find_injective_permutation() ;
                PRINT(std::get<1>(t));
                builder.add( hs 
        }
        #endif


        #if 0
        std::vector< std::vector< holdem_id> > stack;
        stack.emplace_back();

        for(size_t i=0; i!= players.size(); ++i){
                decltype(stack) next_stack;
                auto const& hand_set =  holdem_class_decl::get( players[i] ).get_hand_set() ;
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
                auto t =  permutate_for_the_better( s ) ;
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
