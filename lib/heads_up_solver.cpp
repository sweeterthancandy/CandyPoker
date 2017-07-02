#include "ps/heads_up_solver.h"
#include "ps/heads_up.h"

#include <numeric>
#include <boost/timer/timer.hpp>
#include <thread>
#include <future>

namespace ps{


/*
                
                                        <>
                                 ______/  \______
                                /                \
                             <SB Push>         <SB Fold>
                         ____/     \_____
                        /                \
                    <BB Call>          <BB Fold>


                               +-----------+
                               |Total Value|
                               +-----------+
                                
                        <SB fold>           = -sb
                        <SB push | BB fold> = bb
                        <SB push | BB call> = 2 S Eq - S  
                                            = S( 2 Eq - 1 )

 */

// given hands
//    {sb_id, bb_id}
double calc_detail(calc_context& ctx)
{
        /*
                                         <root>
                                    ______/  \______
                                   /                \
                              <sb_push>          <sb_fold>
                         ____/        \_____
                        /                   \
               <sb_push_bb_call>      <sb_push_bb_fold>

         */
        struct sb_push{
                struct sb_push__bb_call{
                        double operator()(calc_context& ctx)const{
                                auto equity{ctx.cec->visit_boards(std::vector<ps::holdem_class_id>{ ctx.sb_id,ctx.bb_id }).equity()};
                                return ctx.eff_stack * ( 2 * equity - 1 );
                        }
                };
                struct sb_push__bb_fold{
                        double operator()(calc_context& ctx)const{
                                return +ctx.bb;
                        }
                };
                double operator()(calc_context& ctx)const{
                        if( ctx.bb_call_strat[ctx.bb_id] == 0.0 )
                                return bb_fold_(ctx);
                        return 
                                (  ctx.bb_call_strat[ctx.bb_id]) * bb_call_(ctx) +
                                (1-ctx.bb_call_strat[ctx.bb_id]) * bb_fold_(ctx);
                }
                sb_push__bb_call bb_call_;
                sb_push__bb_fold bb_fold_;
        };
        struct sb_fold{
                double operator()(calc_context& ctx)const{
                        return -ctx.sb;
                }
        };
        struct root{
                double operator()(calc_context& ctx)const{

                        // short-circuit
                        if( ctx.sb_push_strat[ctx.sb_id] == 0.0 )
                                return sb_fold_(ctx);

                        return 
                                (  ctx.sb_push_strat[ctx.sb_id]) * sb_push_(ctx) +
                                (1-ctx.sb_push_strat[ctx.sb_id]) * sb_fold_(ctx);
                }
                sb_push sb_push_;
                sb_fold sb_fold_;
        };
        static root root_;
        return root_(ctx);
}
double calc( class_equity_cacher& cec,
                   hu_strategy const& sb_push_strat,
                   hu_strategy const& bb_call_strat,
                   double eff_stack, double sb, double bb)
{
        calc_context ctx = {
                &cec,
                sb_push_strat,
                bb_call_strat,
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        return calc(ctx);
}
double calc(calc_context& ctx){
        double sigma{0.0};
        for(ctx.sb_id = 0; ctx.sb_id != 169;++ctx.sb_id){
                for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                        auto p{ holdem_class_decl::prob(ctx.sb_id, ctx.bb_id) };
                        sigma += calc_detail(ctx) * p;
                }
                        //PRINT(sigma);
        }
        return sigma;
}

hu_strategy solve_hu_push_fold_bb_maximal_exploitable__ev_brute(ps::class_equity_cacher& cec,
                                                                hu_strategy const& sb_push_strat,
                                                                double eff_stack, double sb, double bb)
{
        hu_strategy result{0.0};
        calc_context ctx = {
                &cec,
                sb_push_strat,
                hu_strategy{0.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                double value_fold{0.0};
                double value_call{0.0};
                for(ctx.sb_id = 0; ctx.sb_id != 169;++ctx.sb_id){
                        double p{holdem_class_decl::prob(ctx.sb_id, ctx.bb_id)};
                        value_fold += p * calc_detail(ctx);
                        ctx.bb_call_strat[ctx.bb_id] = 1;
                        value_call += p * calc_detail(ctx);
                        ctx.bb_call_strat[ctx.bb_id] = 0;
                }
                // negative because in bb
                if( value_call < value_fold ){
                        result[ctx.bb_id] = 1;
                }
        }
        return std::move(result);
}
hu_strategy solve_hu_push_fold_sb_maximal_exploitable__ev_brute(ps::class_equity_cacher& cec,
                                               hu_strategy const& bb_call_strat,
                                               double eff_stack, double sb, double bb)
{
        hu_strategy result{0.0};
        calc_context ctx = {
                &cec,
                hu_strategy{0.0},
                bb_call_strat,
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        #if 0
        for(size_t sb_id = 0; sb_id != 169;++sb_id){
                auto value_fold{ calc(ctx) };
                ctx.sb_push_strat[sb_id] = 1;
                auto value_push{ calc(ctx) };
                if( value_fold < value_push )
                        result[sb_id] = 1;
                ctx.sb_push_strat[sb_id] = 0;
        }
        return std::move(result);
        #endif
        for(ctx.sb_id = 0; ctx.sb_id != 169;++ctx.sb_id){
                double value_fold{0.0};
                double value_push{0.0};
                for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                        double p{holdem_class_decl::prob(ctx.sb_id, ctx.bb_id)};
                        value_fold += p * calc_detail(ctx);
                        ctx.sb_push_strat[ctx.sb_id] = 1;
                        value_push += p * calc_detail(ctx);
                        ctx.sb_push_strat[ctx.sb_id] = 0;
                }
                if( value_push > value_fold ){
                        result[ctx.sb_id] = 1;
                }
        }
        return std::move(result);
}

hu_strategy solve_hu_push_fold_bb_maximal_exploitable__ev(ps::class_equity_cacher& cec,
                                               hu_strategy const& sb_push_strat,
                                               double eff_stack, double sb, double bb)
{
        hu_strategy bb_call_strat{0.0};
        calc_context call_ctx = {
                &cec,
                sb_push_strat,
                hu_strategy{1.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        calc_context fold_ctx = {
                &cec,
                sb_push_strat,
                hu_strategy{0.0},
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        for(call_ctx.bb_id = 0; call_ctx.bb_id != 169;++call_ctx.bb_id){
                double value_call{0.0};
                double value_fold{0.0};
                double factor{0.0};
                for(call_ctx.sb_id = 0; call_ctx.sb_id != 169;++call_ctx.sb_id){
                        fold_ctx.sb_id = call_ctx.sb_id;
                        fold_ctx.bb_id = call_ctx.bb_id;
                        
                        auto weight{holdem_class_decl::weight(call_ctx.sb_id, call_ctx.bb_id)};
                        value_call += weight * calc_detail(call_ctx);
                        value_fold += weight * calc_detail(fold_ctx);
                        factor     += weight;
                }
                value_call /= factor;
                value_fold /= factor;

                auto bb_hand{ holdem_class_decl::get(call_ctx.bb_id)};
                //PRINT_SEQ((bb_hand)(value_call)(value_fold));
                // negative becuase calc_detail is orientated for sb
                if( value_call < value_fold){
                        bb_call_strat[call_ctx.bb_id] = 1.0;
                }
        }
        return std::move(bb_call_strat);
}

hu_strategy solve_hu_push_fold_sb_maximal_exploitable__ev(ps::class_equity_cacher& cec,
                                               hu_strategy const& bb_call_strat,
                                               double eff_stack, double sb, double bb)
{
        hu_strategy sb_push_strat{0.0};
        calc_context push_ctx = {
                &cec,
                hu_strategy{1.0},
                bb_call_strat,
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        calc_context fold_ctx = {
                &cec,
                hu_strategy{0.0},
                bb_call_strat,
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        for(push_ctx.sb_id = 0; push_ctx.sb_id != 169;++push_ctx.sb_id){
                double value_push{0.0};
                double value_fold{0.0};
                double factor{0.0};
                for(push_ctx.bb_id = 0; push_ctx.bb_id != 169;++push_ctx.bb_id){
                        fold_ctx.sb_id = push_ctx.sb_id;
                        fold_ctx.bb_id = push_ctx.bb_id;
                        
                        auto weight{holdem_class_decl::weight(push_ctx.sb_id, push_ctx.bb_id)};
                        value_push += weight * calc_detail(push_ctx);
                        value_fold += weight * calc_detail(fold_ctx);
                        factor     += weight;

                }
                value_push /= factor;
                value_fold /= factor;
                auto sb_hand{ holdem_class_decl::get(push_ctx.sb_id)};
                //PRINT_SEQ((sb_hand)(value_push)(value_fold));
                if( value_push > value_fold ){
                        sb_push_strat[push_ctx.sb_id] = 1.0;
                }
        }
        return std::move(sb_push_strat);
}


hu_strategy solve_hu_push_fold_bb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               hu_strategy const& sb_push_strat,
                                               double eff_stack, double sb, double bb)
{
        struct context{
                class_equity_cacher* cec;
                hu_strategy sb_push_strat;
                hu_strategy bb_call_strat;
                double eff_stack;
                double sb;
                double bb;
                holdem_class_id bb_id;
                
                hu_strategy debug;
        };
        /*
                                         <solver>
                                    ______/  \______
                                   /                \
                              <call>              <fold>

         */
        struct fold{
                double operator()(context& ctx)const{
                        return 0;
                        //return -ctx.bb;
                }
        };
        struct call{
                double operator()(context& ctx)const{
                        hu_fresult_t res;
                        for(holdem_class_id sb_id{0}; sb_id != 169;++sb_id){
                                hu_fresult_t tmp{ctx.cec->visit_boards(std::vector<ps::holdem_class_id>{ ctx.bb_id, sb_id })};
                                auto weight{ctx.sb_push_strat[sb_id]};
                                
                                tmp.times(weight);
                                res.append(tmp);
                                
                        }

                        // edge case, can return anything here probably
                        if( res.sigma() == 0 )
                                return 0.0;
                        
                        //PRINT_SEQ((ctx.eff_stack)(ctx.sb)(ctx.bb));

                        double ev{ ctx.eff_stack * 2 * res.equity() - ( ctx.eff_stack - ctx.bb ) };
                        //         \----equity of pot to win -----/   \------cost of bet-------/

                        ctx.debug[ctx.bb_id] = ev;
                        return ev;
                }
        };
        struct solver{
                void operator()(context& ctx)const{
                        //ctx.debug[ctx.bb_id] = ev_call;
                        if( call_(ctx) > fold_(ctx) ){
                                ctx.bb_call_strat[ctx.bb_id] = 1.0;
                        }
                }
        private:
                call call_;
                fold fold_;
        };

        context ctx = {
                &cec,
                sb_push_strat,
                hu_strategy{0.0},
                eff_stack,
                sb,
                bb,
                0
        };
        solver solver_;


        for(holdem_class_id bb_id{0}; bb_id != 169;++bb_id){
                ctx.bb_id = bb_id;
                solver_(ctx);
        }
        //std::cout << "HERE is BB Call diff\n";
        //ctx.debug.display();
        #if 0
        std::cout << "HERE is BB Strat\n";
        ctx.bb_call_strat.display();
        #endif
        return std::move(ctx.bb_call_strat);
} 
hu_strategy solve_hu_push_fold_sb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               hu_strategy const& bb_call_strat,
                                               double eff_stack, double sb, double bb)
{
        struct context{
                class_equity_cacher* cec;
                hu_strategy sb_push_strat;
                hu_strategy bb_call_strat;
                double eff_stack;
                double sb;
                double bb;
                holdem_class_id bb_id;
                holdem_class_id sb_id;
                hu_strategy debug;
        };
        struct sb_push__bb_call{
                double operator()(context& ctx)const{
                        auto equity{ctx.cec->visit_boards(std::vector<ps::holdem_class_id>{ ctx.sb_id, ctx.bb_id }).equity()};
                        return ctx.eff_stack * 2 *  equity - ( ctx.eff_stack -  ctx.sb );
                        //     \- reuity of pot to win  -/   \--- cost of bet  --------/
                }
        };
        struct sb_push__bb_fold{
                double operator()(context& ctx)const{
                        return ctx.sb + ctx.bb;
                }
        };
        struct sb_push{
                double operator()(context& ctx)const{

                        hu_fresult_t agg;
                        for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                                agg.append(ctx.cec->visit_boards(std::vector<ps::holdem_class_id>{ ctx.sb_id, ctx.bb_id }));
                        }
                        auto ret{ ctx.eff_stack * 2 *  agg.equity() - ( ctx.eff_stack -  ctx.sb ) };
                        PRINT_SEQ((ctx.eff_stack)(agg.equity())(ctx.sb)( ctx.eff_stack * 2 *  agg.equity() - ( ctx.eff_stack -  ctx.sb ) ));
                        ctx.debug[ctx.sb_id] = ret;
                        return ret;
                }
        private:
                sb_push__bb_call bb_call_;
                sb_push__bb_fold bb_fold_;
        };
        #if 0
        struct sb_push{
                double operator()(context& ctx)const{
                        double sigma{0.0};
                        double factor{0.0};

                        for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                                auto ev_bb_call{ bb_call_(ctx) };
                                auto ev_bb_fold{ bb_fold_(ctx) };
                                //PRINT_SEQ((ev_bb_call)(ev_bb_fold));
                                double ev_bb{
                                           ctx.bb_call_strat[ctx.bb_id]  * ev_bb_call +
                                        (1-ctx.bb_call_strat[ctx.bb_id]) * ev_bb_fold};
                                auto weight{holdem_class_decl::weight(ctx.sb_id, ctx.bb_id)};
                                sigma  += weight * ev_bb;
                                factor += weight;
                        }
                        ctx.debug[ctx.sb_id] = ev_
                        sigma /= factor;

                        return sigma;
                }
        private:
                sb_push__bb_call bb_call_;
                sb_push__bb_fold bb_fold_;
        };
        #endif
        struct sb_fold{
               double operator()(context& ctx)const{
                        //return ctx.sb;
                        return 0;
                }
        };
        struct solver{
                void operator()(context& ctx)const{
                        auto ev_push{ sb_push_(ctx) };
                        auto ev_fold{ sb_fold_(ctx) };
                        //PRINT_SEQ((ev_push)(ev_fold));
                        if( ev_push > ev_fold )
                                ctx.sb_push_strat[ctx.sb_id] = 1.0;
                }
        private:
                sb_push sb_push_;
                sb_fold sb_fold_;
        };
        context ctx = {
                &cec,
                hu_strategy{0.0},
                bb_call_strat,
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        solver solver_;

        for(ctx.sb_id = 0; ctx.sb_id != 169;++ctx.sb_id){
                solver_(ctx);
        }
        
        //std::cout << "HERE is SB Push diff\n";
        ctx.debug.display();
        return std::move(ctx.sb_push_strat);
} 

hu_strategy solve_hu_push_fold_sb(ps::class_equity_cacher& cec,
                           double eff_stack, double sb, double bb){
        double alpha{0.3};
        hu_strategy sb_strat{1.0};

        std::set< hu_strategy > circular_set;

        for(size_t i=0;i < 20;++i){
                boost::timer::auto_cpu_timer at;

                auto bb_me{solve_hu_push_fold_bb_maximal_exploitable(cec,
                                                                     sb_strat,
                                                                     eff_stack,
                                                                     sb,
                                                                     bb)};

                double ev{calc(cec, sb_strat, bb_me, eff_stack, sb, bb)};

                auto sb_me{solve_hu_push_fold_sb_maximal_exploitable(cec,
                                                                     bb_me,
                                                                     eff_stack,
                                                                     sb,
                                                                     bb)};
                #if 0
                auto sb_me{solve_hu_push_fold_sb_maximal_exploitable(cec,
                                                                     bb_me,
                                                                     eff_stack,
                                                                     sb,
                                                                     bb)};
                auto sb_me_alt{solve_hu_push_fold_sb_maximal_exploitable__ev(cec,
                                                                     bb_me,
                                                                     eff_stack,
                                                                     sb,
                                                                     bb)};
                PRINT( (sb_me - sb_me_alt).norm() );
                (sb_me - sb_me_alt).display();
                #endif
                
                double ev_{calc(cec, sb_me, bb_me, eff_stack, sb, bb)};

                std::cout << "BB COUNTER\n";
                bb_me.display();
                std::cout << "SB COUNTER COUNTER\n";
                sb_me.display();

                auto sb_next{ sb_strat * ( 1 - alpha) + sb_me * alpha };
                auto sb_norm{ (sb_next - sb_strat).norm() };
                sb_strat = std::move(sb_next);
                double ev_d{ std::fabs(ev - ev_) };
                
                PRINT_SEQ((sb_norm)(ev)(ev_)(ev_d));

                if( circular_set.count(sb_strat) ){
                        std::cerr << "ok circular!\n";
                        break;
                }
                circular_set.insert(sb_strat);

                sb_strat.display();

                //
                if( ( ev_d ) < 1e-3 ){
                        std::cout << "_ev_d_break_\n";
                        break;
                }
                if( ( sb_norm ) < 1e-5 ){
                        std::cout << "_break_\n";
                        break;
                }
        }
        return std::move(sb_strat);
}


void make_heads_up_table(){
        using namespace ps;

        equity_cacher ec;
        ec.load("cache.bin");
        class_equity_cacher cec(ec);
        cec.load("hc_cache.bin");

        double bb{1.0};
        double sb{0.5};
                
        hu_strategy sb_table{0.0};
        hu_strategy bb_table{0.0};

        std::vector<
                std::tuple<
                        double, // stack
                        std::future<hu_strategy>
                >
        > results;

        for( double eff_stack{1.0};eff_stack < 20;eff_stack += 0.5){

                auto work = [&cec, eff_stack,sb,bb]()->hu_strategy{
                        auto sb_strat{solve_hu_push_fold_sb(cec, eff_stack, sb, bb)};
                        return std::move(sb_strat);
                };
                std::packaged_task<hu_strategy()> pt(work);
                results.emplace_back( eff_stack, pt.get_future());
                std::thread{std::move(pt)}.detach();



        }
        for(auto& r : results ){
                std::get<1>(r).wait();
                auto eff_stack{ std::get<0>(r) };
                auto sb_strat{ std::get<1>(r).get() };
                auto bb_strat{solve_hu_push_fold_bb_maximal_exploitable(cec,
                                                                        sb_strat,
                                                                        eff_stack,
                                                                        sb,
                                                                        bb)};
                for(size_t i{0};i!=169;++i){
                        if( sb_strat[i] < 1e-3 )
                                continue;
                        if( std::fabs(1.0 - sb_strat[i]) < 1e-3 )
                                sb_table[i] = eff_stack;
                }
                for(size_t i{0};i!=169;++i){
                        if( bb_strat[i] < 1e-3 )
                                continue;
                        if( std::fabs(1.0 - bb_strat[i]) < 1e-3 )
                                bb_table[i] = eff_stack;
                }
        }
        std::cout << "------------ sb push ----------------\n";
        sb_table.display();
        std::cout << "------------ bb call ----------------\n";
        bb_table.display();

}
} // ps
