#include "ps/heads_up.h"
#include "ps/detail/print.h"

#include <numeric>
#include <boost/timer/timer.hpp>

using namespace ps;
using namespace ps::frontend;

/*
        This is the solve the calling range in the sb given that the bb
        is showing with strat 
                hero_strat
 */

struct hu_strategy{
        hu_strategy(double fill){
                for(size_t i{0};i!=169;++i){
                        vec_[i] = fill;
                }
        }
        auto begin()const{ return vec_.begin(); }
        auto end()const{ return vec_.end(); }
        double& operator[](size_t idx){return vec_[idx]; }
        double const& operator[](size_t idx)const{return vec_[idx]; }
        void check(){
                for(size_t i{0};i!=169;++i){
                        if ( !( 0 <= vec_[i] && vec_[i] <= 1.0 && "not a strat") ){
                                std::cerr << "FAILED\n";
                                return;
                        }
                }
        }
        hu_strategy& operator*=(double val){
                for(size_t i{0};i!=169;++i){
                        vec_[i] *= val;
                }
                return *this;
        }
        hu_strategy operator*(double val){
                hu_strategy result{*this};
                result *= val;
                return std::move(result);
        }
        hu_strategy& operator+=(hu_strategy const& that){
                for(size_t i{0};i!=169;++i){
                        vec_[i] += that.vec_[i];
                }
                return *this;
        }
        hu_strategy operator+(hu_strategy const& that){
                hu_strategy result{*this};
                result += that;
                return std::move(result);
        }
        hu_strategy& operator-=(hu_strategy const& that){
                for(size_t i{0};i!=169;++i){
                        vec_[i] -= that.vec_[i];
                }
                return *this;
        }
        hu_strategy operator-(hu_strategy const& that){
                hu_strategy result{*this};
                result -= that;
                return std::move(result);
        }
        double norm()const{
                double result{0.0};
                for(size_t i{0};i!=169;++i){
                        result = std::max(result, std::fabs(vec_[i]));
                }
                return result;
        }
        friend std::ostream& operator<<(std::ostream& ostr, hu_strategy const& strat){
                return ostr << ps::detail::to_string(strat.vec_);
        }
private:
        std::array<double, 169> vec_;
};

auto solve_hu_push_fold_bb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               hu_strategy const& villian_strat,
                                               double eff_stack, double bb, double sb)
{
        hu_strategy counter_strat{0.0};

        for(holdem_class_id x{0}; x != 169;++x){
                /*
                 *  We call iff
                 *
                 *    EV<Call x| villian push> >= 0
                 *  =>
                 *    
                 *
                 */

                double win{0};
                double lose{0};
                double draw{0};
                // weight average again villians range
                for(holdem_class_id y{0}; y != 169;++y){
                        auto p{villian_strat[y]};
                        if( p == 0.0 )
                                continue;

                        auto const& sim{cec.visit_boards(std::vector<ps::holdem_class_id>{ x,y })};
                        win  += sim.win  * p;
                        lose += sim.lose * p;
                        draw += sim.draw * p;
                }
                double sigma{ win + lose + draw };
                double equity{ (win + draw / 2.0 ) / sigma};

                /*
                        Ev<Call> = equity * sizeofpot - costofcall
                        EV<Fold> = 0
                */
                double ev_call{ equity * 2 * ( eff_stack + bb ) - eff_stack };
                //double ev_fold{ 0.0 };
                auto hero{ holdem_class_decl::get(x) };
                //PRINT_SEQ((hero)(sigma)(equity)(ev_call));

                if( ev_call >= 0 ){
                        counter_strat[x] = 1.0;
                }

        }
        //PRINT_SEQ(( std::accumulate(counter_strat.begin(), counter_strat.end(), 0.0) / 169 ));
        return std::move(counter_strat);
} 
auto solve_hu_push_fold_sb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               hu_strategy const& villian_strat,
                                               double eff_stack, double bb, double sb)
{
        hu_strategy counter_strat{0.0};
        for(holdem_class_id x{0}; x != 169;++x){
                /*
                 *  We call iff
                 *
                 *    EV<push x| villian call with S> >= 0
                 */

                double win{0};
                double lose{0};
                double draw{0};

                double win_preflop{0};
                
                double sigma{0};

                // weight average again villians range
                for(holdem_class_id y{0}; y != 169;++y){
                        auto p{villian_strat[y]};

                        auto const& sim{cec.visit_boards(std::vector<ps::holdem_class_id>{ x,y })};
                        win  += sim.win  * p;
                        lose += sim.lose * p;
                        draw += sim.draw * p;

                        auto total{ sim.win + sim.lose + sim.draw };
                        sigma += total;
                        win_preflop += total * ( 1- p );

                }
                double call_sigma{ win + lose + draw };
                double call_equity{ (win + draw / 2.0 ) / call_sigma};
                
                double call_p{call_sigma / sigma };

                double fold_p{win_preflop / sigma};

                /*
                        Ev<Push> = Ev<Villan calls | Push> + Ev<Villian fold | Push>
                */
                double ev_push_call{ call_equity * 2 * ( eff_stack + sb ) - eff_stack };

                double ev_push{ call_p * ev_push_call + fold_p * ( sb + bb ) }; 
                
                //auto hero{ holdem_class_decl::get(x) };

                //PRINT_SEQ((hero)(sigma)(call_equity)(call_p)(fold_p)(call_p+fold_p)(ev_push_call)(ev_push));

                if( ev_push >= 0 ){
                        counter_strat[x] = 1.0;
                }

        }
        return std::move(counter_strat);
} 



int main(){
        using namespace ps;
        using namespace ps::frontend;

        equity_cacher ec;
        ec.load("cache.bin");
        class_equity_cacher cec(ec);
        cec.load("hc_cache.bin");

        #if 0
        auto _AKo{ holdem_class_decl::parse("AKo")};
        auto _33 { holdem_class_decl::parse("33" )};
        auto _JTs{ holdem_class_decl::parse("JTs")};

        PRINT( cec.visit_boards(
                        std::vector<ps::holdem_class_id>{
                                _AKo, _JTs } ) );
                                #endif
        double eff_stack{10.0};
        for(;;eff_stack += 1.0){
                double alpha{0.5};
                hu_strategy sb_strat{1.0};
                hu_strategy bb_strat{1.0};
                double bb{1.0};
                double sb{0.5};
                for(size_t count=0;;++count){

                        auto bb_me{solve_hu_push_fold_bb_maximal_exploitable(cec,
                                                                             sb_strat,
                                                                             eff_stack,
                                                                             bb,
                                                                             sb)};
                        auto bb_next{ bb_strat * ( 1 - alpha) + bb_me * alpha };
                        auto bb_norm{ (bb_next - bb_strat).norm() };
                        bb_strat = std::move(bb_next);

                        auto sb_me{solve_hu_push_fold_sb_maximal_exploitable(cec,
                                                                             bb_strat,
                                                                             eff_stack,
                                                                             bb,
                                                                             sb)};
                        auto sb_next{ sb_strat * ( 1 - alpha) + sb_me * alpha };
                        auto sb_norm{ (sb_next - sb_strat).norm() };
                        sb_strat = std::move(sb_next);

                        #if 0
                        PRINT(bb_strat);
                        PRINT(sb_strat);
                        #endif

                        #if 0
                        for(size_t i{0};i!=169;++i){
                                if( sb_strat[i] < 1e-3 )
                                        sb_strat[i] = 0.0;
                                else if( std::fabs(1.0 - sb_strat[i]) > 1e-3 )
                                        sb_strat[i] = 1.0;
                        }
                        #endif
                        //PRINT_SEQ((::ps::detail::to_string(sb_strat)));
                        //
                        PRINT_SEQ((sb_norm)(bb_norm));
                        if( ( sb_norm + bb_norm ) < 1e-5 ){
                                std::cout << "_break_\n";
                                break;
                        }

                }
                std::cout << "-------- BB " << eff_stack << "--------\n";
                for(size_t i{0};i!=169;++i){
                        if( sb_strat[i] < 1e-3 )
                                continue;
                        if( std::fabs(1.0 - sb_strat[i]) > 1e-3 )
                                std::cout << sb_strat[i] << " ";
                        std::cout << holdem_class_decl::get(i) << " ";
                }
                std::cout << "\n";
        }
        #if 0
        for(size_t i{0};i!=3;++i){
                boost::timer::auto_cpu_timer at;
                for(holdem_class_id x{0}; x != 169;++x){
                        for(holdem_class_id y{0}; y != 169;++y){
                                auto hero{ holdem_class_decl::get(x) };
                                auto villian{ holdem_class_decl::get(y) };
                                cec.visit_boards( std::vector<ps::holdem_class_id>{ x,y } );
                                #if 0
                                PRINT_SEQ((hero)(villian)( cec.visit_boards(
                                                std::vector<ps::holdem_class_id>{
                                                        x,y } ) ));
                                                        #endif
                        }
                }
        }
        #endif

}
