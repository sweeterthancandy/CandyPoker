#include "ps/calculator_detail.h"
#include "ps/calculator.h"
#include "ps/heads_up.h"
#include "ps/frontend.h"

#include <boost/timer/timer.hpp>

using namespace ps;
using namespace ps::detail;


void test0(){
        equity_calc_detail ecd;
        equity_cacher ec;
        class_equity_cacher cec{ec};
        ec.load("cache.bin");
        basic_calculator_N<2> other_ec{&ecd};

        PRINT(  ec.visit_boards( std::vector<ps::holdem_id>{
                                 holdem_hand_decl::parse("AdKs").id(),
                                 holdem_hand_decl::parse("TsTh").id()} ) );
        PRINT(  ec.visit_boards( std::vector<ps::holdem_id>{
                                 holdem_hand_decl::parse("TsTh").id(),
                                 holdem_hand_decl::parse("AdKs").id()
                                 } ) );
        PRINT( other_ec.calculate( 
                        std::array<ps::holdem_id, 2>{
                                 holdem_hand_decl::parse("AdKs").id(),
                                 holdem_hand_decl::parse("TsTh").id()
                                 } ) );
        PRINT( other_ec.calculate( std::array<ps::holdem_id, 2>{
                                 holdem_hand_decl::parse("TsTh").id(),
                                 holdem_hand_decl::parse("AdKs").id()
                                 } ) );

}

#if 1
void test1(){
        equity_calc_detail ecd;

        equity_cacher ec;
        ec.load("cache.bin");

        basic_calculator_N<2> other_ec{&ecd};
        size_t count{0};
        boost::timer::auto_cpu_timer at;
        //other_ec.load("new_cache.bin");
        for(ps::holdem_id i=0; i!= holdem_hand_decl::max_id;++i){
                for(ps::holdem_id j=0; j!= holdem_hand_decl::max_id;++j){
                        if( disjoint(holdem_hand_decl::get(i),holdem_hand_decl::get(j)) ){
                                auto left{ other_ec.calculate(
                                        std::array<ps::holdem_id, 2>{ i, j } ) };
                                #if 1
                                auto right{ ec.visit_boards(
                                        std::vector<ps::holdem_id>{ i, j } ) };
                                        #endif
                                #if 1
                                PRINT(right);
                                PRINT(left);
                                PRINT( std::fabs( left.player(0).equity() - right.equity() ) );
                                #endif

                                #if 0
                                if( ++count % 100 == 0 ){
                                        std::cerr << "saving\n";
                                        other_ec.save("new_cache.bin");
                                        other_ec.save("new_cache.binn");
                                        std::cerr << "done\n";
                                }
                                #endif
                                PRINT(count);
                                if( ++count == 500 )
                                        goto cleanup;

                        }
                }
        }
cleanup:
        other_ec.save("new_cache.bin");
}
#endif



#if 1
void test2(){
        equity_calc_detail ecd;


        basic_calculator_N<3> other_ec{&ecd};
        boost::timer::auto_cpu_timer at;
        for(ps::holdem_id i=0; i!= holdem_hand_decl::max_id;++i){
                for(ps::holdem_id j=0; j!= holdem_hand_decl::max_id;++j){
                        for(ps::holdem_id k=0; k!= holdem_hand_decl::max_id;++k){
                                if( disjoint(holdem_hand_decl::get(i),
                                             holdem_hand_decl::get(j),
                                             holdem_hand_decl::get(k)) ){
                                        auto left{ other_ec.calculate(
                                                std::array<ps::holdem_id, 3>{ i, j, k } ) };
                                        PRINT(left);
                                }
                        }
                }
        }
}
#endif

#if 1
void test3(){
        equity_calc_detail ecd;

        using hand_calc_t = 
        basic_calculator_N<3>;

        using class_calc_t = 
        basic_class_calculator_N<3>;
        
        hand_calc_t other_ec{&ecd};
        class_calc_t cother_ec{&other_ec};
        auto left{ cother_ec.calculate(
                std::array<ps::holdem_class_id, 3>{ 
                        holdem_class_decl::parse("AKs").id(),
                        holdem_class_decl::parse("KQs").id(),
                        holdem_class_decl::parse("QJs").id() } ) };
        PRINT(left);
        return;

        boost::timer::auto_cpu_timer at;

        for(ps::holdem_class_id i=0; i!= holdem_class_decl::max_id;++i){
                for(ps::holdem_class_id j=0; j!= holdem_class_decl::max_id;++j){
                        for(ps::holdem_class_id k=0; k!= holdem_class_decl::max_id;++k){
                                auto left{ cother_ec.calculate(
                                                std::array<ps::holdem_class_id, 3>{ i, j, k } ) };
                                PRINT(left);
                        }
                }
        }
}
#endif


#if 1
void test4(){
        equity_calc_detail ecd;

        using hand_calc_t = 
        basic_calculator_N<4>;

        using class_calc_t = 
        basic_class_calculator_N<4>;
        
        hand_calc_t other_ec{&ecd};
        class_calc_t cother_ec{&other_ec};
        auto left{ cother_ec.calculate(
                std::array<ps::holdem_class_id, 4>{ 
                        holdem_class_decl::parse("AKs").id(),
                        holdem_class_decl::parse("QJs").id(),
                        holdem_class_decl::parse("T9s").id(),
                        holdem_class_decl::parse("87s").id()
                        } ) };
        PRINT(left);
        return;

}
#endif

void test5(){
        calculater calc;
        auto left{ calc.calculate_class_equity(
                std::array<ps::holdem_class_id, 4>{ 
                        holdem_class_decl::parse("AKs").id(),
                        holdem_class_decl::parse("QJs").id(),
                        holdem_class_decl::parse("T9s").id(),
                        holdem_class_decl::parse("87s").id()
                        } ) };
        PRINT(left);

}


int main(){
        
        size_t sigma{0};

        std::vector<std::vector<ps::holdem_id> > world;
        #if 0
        frontend::range p0;
        frontend::range p1;
        p0 = frontend::percent(100);
        p1 = frontend::percent(100);

        tree_range root{ std::vector<frontend::range>{p0, p1} };
        
        for( auto const& c : root.children ){
                for( auto d : c.children ){
                        ++sigma;
                        auto p{ permutate_for_the_better(d.players) };
                        world.insert( std::get<1>(p) );
                }
        }
        #endif
        detail::visit_combinations<2>(
                [&](auto a, auto b){
                world.emplace_back(
                        std::vector<holdem_id>{
                                static_cast<holdem_id>(a),
                                static_cast<holdem_id>(b)});
        }, detail::true_, holdem_class_decl::max_id -1 );

        PRINT(world.size());

        auto iter{world.begin()};
        auto end{world.end()};

        
        std::mutex mtx;
        std::mutex result_mtx;
        
        std::atomic_int done{0};
        
        std::vector<std::thread> tg;
        std::vector<calculater> result;
        for(size_t i=0; i!= std::thread::hardware_concurrency()*2; ++i){
                tg.emplace_back( [&]()mutable{
                        calculater calc;
                        for(;;){
                                mtx.lock();
                                if( iter == end ){
                                        mtx.unlock();
                                        break;
                                }
                                auto first{iter};
                                const size_t batch_size{50};
                                for(size_t c=0; c != batch_size && iter!=end;++c,++iter);
                                auto last{iter};
                                mtx.unlock();
                                
                                for(;first!=last;++first){
                                        calc.calculate_class_equity( *first );
                                        ++done;
                                }
                                PRINT_SEQ((sigma)(world.size())(done));
                                #if 0
                                if( done > 100 )
                                        break;
                                #endif
                        }
                        std::lock_guard<std::mutex> lock(result_mtx);
                        result.push_back(std::move(calc));
                } );
        }
        for( auto& t : tg )
                t.join();
        calculater aggregate;
        for( auto const& r : result )
                aggregate.append(r);
        aggregate.save("newcache.bin");
        aggregate.save("newcache.bin_");
                        
        PRINT_SEQ((sigma)(world.size())(done));
}
