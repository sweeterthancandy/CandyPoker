#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"


#include <boost/timer/timer.hpp>

using namespace ps;

struct holdem_class_strategy{
        holdem_class_strategy(double fill = 1.0){
                for(size_t i{0};i!=169;++i){
                        vec_[i] = fill;
                }
        }
        auto begin()const{ return vec_.begin(); }
        auto end()  const{ return vec_.end();   }
        double&       operator[](size_t idx)     {return vec_[idx]; }
        double const& operator[](size_t idx)const{return vec_[idx]; }
        void check(){
                for(size_t i{0};i!=169;++i){
                        if ( !( 0 <= vec_[i] && vec_[i] <= 1.0 && "not a strat") ){
                                std::cerr << "FAILED\n";
                                return;
                        }
                }
        }
        holdem_class_strategy& operator*=(double val){
                for(size_t i{0};i!=169;++i){
                        vec_[i] *= val;
                }
                return *this;
        }
        holdem_class_strategy& operator+=(holdem_class_strategy const& that){
                for(size_t i{0};i!=169;++i){
                        vec_[i] += that.vec_[i];
                }
                return *this;
        }
        holdem_class_strategy& operator-=(holdem_class_strategy const& that){
                for(size_t i{0};i!=169;++i){
                        vec_[i] -= that.vec_[i];
                }
                return *this;
        }

        holdem_class_strategy operator*(double val){
                holdem_class_strategy result{*this};
                result *= val;
                return std::move(result);
        }
        holdem_class_strategy operator+(holdem_class_strategy const& that){
                holdem_class_strategy result{*this};
                result += that;
                return std::move(result);
        }
        holdem_class_strategy operator-(holdem_class_strategy const& that){
                holdem_class_strategy result{*this};
                result -= that;
                return std::move(result);
        }
        double sigma(){
                return std::accumulate(vec_.begin(), vec_.end(), 0.0);
        }

        double norm()const{
                double result{0.0};
                for(size_t i{0};i!=169;++i){
                        result = std::max(result, std::fabs(vec_[i]));
                }
                return result;
        }
        friend std::ostream& operator<<(std::ostream& ostr, holdem_class_strategy const& strat){
                return ostr << ps::detail::to_string(strat.vec_);
        }
        // print pretty table
        //
        //      AA  AKs ... A2s
        //      AKo KK
        //      ...     ...
        //      A2o         22
        //
        //
        void display(){
                /*
                        token_buffer[0][0] token_buffer[1][0]
                        token_buffer[0][1]

                        token_buffer[y][x]


                 */
                std::array<
                        std::array<std::string, 13>, // x
                        13                           // y
                > token_buffer;
                std::array<size_t, 13> widths;

                for(size_t i{0};i!=169;++i){
                        auto const& decl =  holdem_class_decl::get(i) ;
                        size_t x{decl.first().id()};
                        size_t y{decl.second().id()};
                        // inverse
                        x = 12 - x;
                        y = 12 - y;
                        if( decl.category() == holdem_class_type::offsuit ){
                                std::swap(x,y);
                        }

                        #if 1
                        //token_buffer[y][x] = boost::lexical_cast<std::string>(vec_[i]);
                        if( vec_[i] == 1.0 ){
                                token_buffer[y][x] = "1";
                        } else if( vec_[i] == 0.0 ){
                                token_buffer[y][x] = "0";
                        } else {
                                token_buffer[y][x] = str(boost::format("%.4f") % vec_[i]);
                        }

                        #else
                        token_buffer[y][x] = boost::lexical_cast<std::string>(decl.to_string());
                        #endif
                }
                for(size_t i{0};i!=13;++i){
                        widths[i] = std::max_element( token_buffer[i].begin(),
                                                      token_buffer[i].end(),
                                                      [](auto const& l, auto const& r){
                                                              return l.size() < r.size(); 
                                                      })->size();
                }

                auto pad= [](auto const& s, size_t w){
                        size_t padding{ w - s.size()};
                        size_t left_pad{padding/2};
                        size_t right_pad{padding - left_pad};
                        std::string ret;
                        if(left_pad)
                               ret += std::string(left_pad,' ');
                        ret += s;
                        if(right_pad)
                               ret += std::string(right_pad,' ');
                        return std::move(ret);
                };
                
                std::cout << "   ";
                for(size_t i{0};i!=13;++i){
                        std::cout << pad( rank_decl::get(12-i).to_string(), widths[i] ) << " ";
                }
                std::cout << "\n";
                std::cout << "  +" << std::string( std::accumulate(widths.begin(), widths.end(), 0) + 13, '-') << "\n";

                for(size_t i{0};i!=13;++i){
                        std::cout << rank_decl::get(12-i).to_string() << " |";
                        for(size_t j{0};j!=13;++j){
                                if( j != 0){
                                        std::cout << " ";
                                }
                                std::cout << pad(token_buffer[j][i], widths[j]);
                        }
                        std::cout << "\n";
                }
        }
        void transform(std::function<double(size_t i, double)> const& t){
                for(size_t i{0};i!=169;++i){
                        vec_[i] = t(i, vec_[i]);
                }
        }
        bool operator<(holdem_class_strategy const& that)const{
                return vec_ < that.vec_;
        }
private:
        std::array<double, 169> vec_;
};

struct calc_context{
        holdem_class_strategy sb_push_strat;
        holdem_class_strategy bb_call_strat;
        double eff_stack;
        double sb;
        double bb;
        holdem_class_id sb_id;
        holdem_class_id bb_id;
};

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
                                static auto const& eval = class_equity_evaluator_factory::get("principal");
                                auto ret = eval.evaluate( holdem_class_vector{  ctx.sb_id,ctx.bb_id } );
                                auto equity = ret->player(0).equity();
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
double calc_impl(calc_context& ctx){
        double sigma{0.0};
        for(ctx.sb_id = 0; ctx.sb_id != 169;++ctx.sb_id){
                for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                        auto p =  holdem_class_decl::prob(ctx.sb_id, ctx.bb_id) ;
                        sigma += calc_detail(ctx) * p;
                }
                        //PRINT(sigma);
        }
        return sigma;
}
double calc( holdem_class_strategy const& sb_push_strat,
             holdem_class_strategy const& bb_call_strat,
             double eff_stack, double sb, double bb)
{
        calc_context ctx = {
                sb_push_strat,
                bb_call_strat,
                eff_stack,
                sb,
                bb,
                0,
                0
        };
        return calc_impl(ctx);
}


holdem_class_strategy solve_hu_push_fold_bb_maximal_exploitable( holdem_class_strategy const& sb_push_strat,
                                                       double eff_stack, double sb, double bb)
{
        /*
                                         <solver>
                                    ______/  \______
                                   /                \
                              <call>              <fold>

         */
        struct fold{
                double operator()(calc_context& ctx)const{
                        return 0;
                        //return -ctx.bb;
                }
        };
        struct call{
                double operator()(calc_context& ctx)const{
                        static auto const& eval = class_equity_evaluator_factory::get("principal");
                        auto agg = std::make_shared<equity_breakdown_matrix_aggregator>(2);
                        for(holdem_class_id sb_id{0}; sb_id != 169;++sb_id){
                                auto ret = eval.evaluate( holdem_class_vector{  ctx.sb_id,ctx.bb_id } );
                                auto weight = ctx.sb_push_strat[sb_id];
                                agg->append_scalar(*ret, weight);
                                

                        }

                        // edge case, can return anything here probably
                        if( agg->sigma() == 0 )
                                return 0.0;
                        
                        //PRINT_SEQ((ctx.eff_stack)(ctx.sb)(ctx.bb));

                        double ev{ ctx.eff_stack * 2 * agg->player(0).equity() - ( ctx.eff_stack - ctx.bb ) };
                        //         \----equity of pot to win -----/   \------cost of bet-------/

                        return ev;
                }
        };
        struct solver{
                void operator()(calc_context& ctx)const{
                        //ctx.debug[ctx.bb_id] = ev_call;
                        if( call_(ctx) > fold_(ctx) ){
                                ctx.bb_call_strat[ctx.bb_id] = 1.0;
                        }
                }
        private:
                call call_;
                fold fold_;
        };

        calc_context ctx = {
                sb_push_strat,
                holdem_class_strategy{0.0},
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
holdem_class_strategy solve_hu_push_fold_sb_maximal_exploitable( holdem_class_strategy const& bb_call_strat,
                                                                 double eff_stack, double sb, double bb)
{
        struct sb_push__bb_call{
                double operator()(calc_context& ctx)const{
                        static auto const& eval = class_equity_evaluator_factory::get("principal");
                        auto ret = eval.evaluate( holdem_class_vector{  ctx.sb_id,ctx.bb_id } );
                        auto equity = ret->player(0).equity();
                        return ctx.eff_stack * 2 *  equity - ( ctx.eff_stack -  ctx.sb );
                        //     \- reuity of pot to win  -/   \--- cost of bet  --------/
                }
        };
        struct sb_push__bb_fold{
                double operator()(calc_context& ctx)const{
                        return ctx.sb + ctx.bb;
                }
        };
        #if 0
        struct sb_push{
                double operator()(calc_context& ctx)const{

                        hu_fresult_t agg;
                        for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                                agg.append(ctx.cec->visit_boards(std::vector<ps::holdem_class_id>{ ctx.sb_id, ctx.bb_id }));
                        }
                        auto ret =  ctx.eff_stack * 2 *  agg.equity() - ( ctx.eff_stack -  ctx.sb ) ;
                        PRINT_SEQ((ctx.eff_stack)(agg.equity())(ctx.sb)( ctx.eff_stack * 2 *  agg.equity() - ( ctx.eff_stack -  ctx.sb ) ));
                        ctx.debug[ctx.sb_id] = ret;
                        return ret;
                }
        private:
                sb_push__bb_call bb_call_;
                sb_push__bb_fold bb_fold_;
        };
        #else
        struct sb_push{
                double operator()(calc_context& ctx)const{
                        double sigma{0.0};
                        double factor{0.0};

                        for(ctx.bb_id = 0; ctx.bb_id != 169;++ctx.bb_id){
                                auto ev_bb_call =  bb_call_(ctx) ;
                                auto ev_bb_fold =  bb_fold_(ctx) ;
                                //PRINT_SEQ((ev_bb_call)(ev_bb_fold));
                                double ev_bb{
                                           ctx.bb_call_strat[ctx.bb_id]  * ev_bb_call +
                                        (1-ctx.bb_call_strat[ctx.bb_id]) * ev_bb_fold};
                                auto weight = holdem_class_decl::weight(ctx.sb_id, ctx.bb_id);
                                sigma  += weight * ev_bb;
                                factor += weight;
                        }
                        sigma /= factor;

                        return sigma;
                }
        private:
                sb_push__bb_call bb_call_;
                sb_push__bb_fold bb_fold_;
        };
        #endif
        struct sb_fold{
               double operator()(calc_context& ctx)const{
                        //return ctx.sb;
                        return 0;
                }
        };
        struct solver{
                void operator()(calc_context& ctx)const{
                        auto ev_push =  sb_push_(ctx) ;
                        auto ev_fold =  sb_fold_(ctx) ;
                        //PRINT_SEQ((ev_push)(ev_fold));
                        if( ev_push > ev_fold )
                                ctx.sb_push_strat[ctx.sb_id] = 1.0;
                }
        private:
                sb_push sb_push_;
                sb_fold sb_fold_;
        };
        calc_context ctx = {
                holdem_class_strategy{0.0},
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
        
        return std::move(ctx.sb_push_strat);
} 

holdem_class_strategy solve_hu_push_fold_sb( double eff_stack, double sb, double bb){
        double alpha{0.3};
        holdem_class_strategy sb_strat{1.0};

        std::set< holdem_class_strategy > circular_set;

        for(size_t i=0;;++i){
                boost::timer::auto_cpu_timer at;

                auto bb_me = solve_hu_push_fold_bb_maximal_exploitable(
                                                                     sb_strat,
                                                                     eff_stack,
                                                                     sb,
                                                                     bb);

                double ev = calc(sb_strat, bb_me, eff_stack, sb, bb);

                auto sb_me = solve_hu_push_fold_sb_maximal_exploitable(
                                                                       bb_me,
                                                                       eff_stack,
                                                                       sb,
                                                                       bb);
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
                
                double ev_{calc(sb_me, bb_me, eff_stack, sb, bb)};

                std::cout << "BB COUNTER\n";
                bb_me.display();
                std::cout << "SB COUNTER COUNTER\n";
                sb_me.display();

                auto sb_next =  sb_strat * ( 1 - alpha) + sb_me * alpha ;
                auto sb_norm =  (sb_next - sb_strat).norm() ;
                sb_strat = std::move(sb_next);
                double ev_d{ std::fabs(ev - ev_) };
                
                PRINT_SEQ((sb_norm)(ev)(ev_)(ev_d));

                if( circular_set.count(sb_strat) ){
                        std::cerr << "ok circular!\n";
                        break;
                }
                circular_set.insert(sb_strat);

                sb_strat.display();

                #if 0
                if( ( ev_d ) < 1e-3 ){
                        std::cout << "_ev_d_break_\n";
                        break;
                }
                #endif
                if( ( sb_norm ) < 1e-5 ){
                        std::cout << "_break_\n";
                        break;
                }
                if( i == 50){
                        std::cout << "_i_break_\n";
                        break;
                }
        }
        return std::move(sb_strat);
}


void make_heads_up_table(){
        using namespace ps;


        double bb{1.0};
        double sb{0.5};
                
        holdem_class_strategy sb_table{0.0};
        holdem_class_strategy bb_table{0.0};

        std::vector<
                std::tuple<
                        double, // stack
                        std::future<holdem_class_strategy>
                >
        > results;

        for( double eff_stack{4.0};eff_stack < 10;eff_stack += 1){

                auto work = [eff_stack,sb,bb]()->holdem_class_strategy{
                        auto sb_strat = solve_hu_push_fold_sb( eff_stack, sb, bb);
                        return std::move(sb_strat);
                };
                std::packaged_task<holdem_class_strategy()> pt(work);
                results.emplace_back( eff_stack, pt.get_future());
                std::thread{std::move(pt)}.detach();



        }
        for(auto& r : results ){
                std::get<1>(r).wait();
                auto eff_stack =  std::get<0>(r) ;
                auto sb_strat =  std::get<1>(r).get() ;
                auto bb_strat = solve_hu_push_fold_bb_maximal_exploitable( sb_strat,
                                                                           eff_stack,
                                                                           sb,
                                                                           bb);
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






int main(){
        try{
                solve_hu_push_fold_sb( 10, .5, 1);
        } catch(std::exception const& e){
                std::cerr << "Caught exception: " << e.what() << "\n";
        }
        return EXIT_FAILURE;
}
