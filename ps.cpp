#include "ps/heads_up_solver.h"

#include <random>
#include <future>
#include <thread>
#include <boost/range/algorithm.hpp>

using namespace ps;

double do_simulation( equity_cacher& ec,
                      class_equity_cacher& cec,
                      hu_strategy const& sb_strat,
                      hu_strategy const& bb_strat,
                      double eff_stack,
                      double sb,
                      double bb,
                      size_t n)
{
        std::default_random_engine gen;
        std::uniform_int_distribution<holdem_class_id> deck(0,51);

        long double ev{0};
        double e_ev{ calc( cec, sb_strat, bb_strat, eff_stack, sb, bb ) };
        size_t i{0};
        for(;i!=n;++i){
                long double sim{0.0};
                std::vector<card_id> deal;
                for(;deal.size() < 4;){
                        auto c{ deck(gen) };
                        if( boost::find(deal, c) == deal.end())
                                deal.emplace_back(c);
                }
                auto const& sb_hand{holdem_hand_decl::get( deal[0], deal[1] ) };
                auto const& bb_hand{holdem_hand_decl::get( deal[2], deal[3] ) };
                
                #if 0
                if( i % 100 == 0 )
                        PRINT_SEQ((i)(ev / i));
                        #endif

                if( sb_strat[sb_hand.class_()] == 0.0 ){
                        ev += ( -sb );
                        continue;
                }
                if( bb_strat[bb_hand.class_()] == 0.0 ){
                        ev += ( +bb );
                        continue;
                }
                auto ret{ ec.visit_boards( std::vector<holdem_id>{ sb_hand.id(), bb_hand.id()})};
                ev += ( eff_stack * ( 2 * ret.equity() - 1 ) );
        }
        PRINT_SEQ((n)(e_ev)(ev / i));
        return ev / i;
}

double simulation( equity_cacher& ec,
                   class_equity_cacher& cec,
                   hu_strategy const& sb_strat,
                   hu_strategy const& bb_strat,
                   double eff_stack,
                   double sb,
                   double bb,
                   size_t n) // n per thread
{
        size_t num_threads{ std::thread::hardware_concurrency() };
        size_t N{0};
        std::vector<std::future<double> > results;
        for(size_t i{0}; i != num_threads ;++i){
                N += n;
                std::packaged_task<double()> task{
                        [&,i](){
                                return do_simulation(ec,
                                                     cec,
                                                     sb_strat,
                                                     bb_strat,
                                                     eff_stack,
                                                     sb,
                                                     bb,
                                                     n);
                        }};
                results.emplace_back( task.get_future() );
                std::thread{std::move(task)}.detach();
        }
        double sigma{0.0};
        for(auto& r : results){
                PRINT(r.get());
                sigma += r.get() / num_threads;
        }
        return sigma;
}

int main(){
        using namespace ps;

        equity_cacher ec;
        ec.load("cache.bin");
        class_equity_cacher cec(ec);
        cec.load("hc_cache.bin");

        double eff_stack{10.0};
        double bb{1.0};
        double sb{0.5};

        hu_strategy{1.0}.display();
                        
        auto sb_strat{solve_hu_push_fold_sb(cec, eff_stack, sb, bb)};
        auto bb_strat{solve_hu_push_fold_bb_maximal_exploitable(cec,
                                                                sb_strat,
                                                                eff_stack,
                                                                sb,
                                                                bb)};
        sb_strat.transform( [](auto i, auto d){
                #if 0
                if( std::fabs( d - 0.0 ) < 1e-3 )
                        return 0.0;
                if( std::fabs( d - 1.0 ) < 1e-3 )
                        return 1.0;
                return d;
                #endif
                // map [0,1] to {0,1}
                if( d < 0.5 )
                        return 0.0;
                return 1.0;

        });

        sb_strat.display();


        PRINT(simulation(ec, cec, sb_strat, bb_strat, eff_stack, sb, bb, 1000));



}
