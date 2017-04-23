#include "ps/symbolic.h"
#include "ps/transforms.h"

#include <type_traits>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>


namespace{
        void run_driver(std::vector<ps::frontend::range> const& players)
        {
                boost::timer::auto_cpu_timer at("driver took %w seconds\n");

                using namespace ps;
                using namespace ps::frontend;
                using namespace ps::transforms;

                symbolic_computation::handle star = std::make_shared<symbolic_range>( players );

                //star->print();

                symbolic_computation::transform_schedular sch;
                calculation_context ctx;


                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          std::make_shared<to_lowest_permutation>() );
                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          std::make_shared<remove_suit_perms>() );
                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          std::make_shared<consolidate_dup_prim>() );
                #if 0
                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          std::make_shared<calc_primitive>(ctx) );
                #endif

                {
                        boost::timer::auto_cpu_timer at("tranforms took %w seconds\n");
                        sch.execute(star);
                }
                return;
                star->print();

                #if 0
                {
                        boost::timer::auto_cpu_timer at;
                        preempt.execute(star);
                }
                #endif

                
                
                //star->print();

        
                decltype( star->calculate(ctx)) ret;
                {
                        boost::timer::auto_cpu_timer at("calculate took %w seconds\n");
                        ret = star->calculate(ctx);
                }

                auto fmtc = [](auto c){
                        static_assert( std::is_integral< std::decay_t<decltype(c)> >::value, "");
                        std::string mem = boost::lexical_cast<std::string>(c);
                        std::string ret;

                        for(size_t i=0;i!=mem.size();++i){
                                if( i % 3 == 0 && i != 0 )
                                        ret += ',';
                                ret += mem[mem.size()-i-1];
                        }
                        return std::string{ret.rbegin(), ret.rend()};
                };

                const char* title_fmt{ "%10s %16s %16s %12s %12s %10s %10s %20s %10s\n" };
                const char* fmt{       "%10s %16s %16s %12s %12s %10s %10s %20s %10.4f\n" };
                std::cout << boost::format(title_fmt)
                        % "n" % "wins" % "tied" % "|tied|" % "draws2" % "draws3" % "sigma" % "equity" % "equity %";
                for(size_t i{0};i!=ret.size1();++i){
                        double tied_weighted{0.0};
                        size_t tied_abs{0};
                        for( size_t j=1; j <= 3;++j){
                                tied_weighted += static_cast<double>(ret(i,j)) / (j+1);
                                tied_abs += ret(i,j);
                        }
                        std::cout << boost::format(fmt)
                                % players[i] % fmtc(ret(i,0))
                                % fmtc(tied_abs) % fmtc(static_cast<int>(tied_weighted))
                                % fmtc(ret(i,1)) % fmtc(ret(i,2)) 
                                % fmtc(ret(i,9)) % fmtc(ret(i,10)) 
                                % ( static_cast<double>(ret(i,10) ) / computation_equity_fixed_prec / ret(i,9) * 100 );
                }
        }
} // anon
 
void test0(){

        using namespace ps;
        using namespace ps::frontend;

        range p0;
        range p1;
        range p2;
        range p3;
        range p4;

        range villian;


        #if 0
        p0 += _AKs;
        p1 += _KQs;
        p2 += _QJs;
        #elif 0
        p0 += _AKo;
        p1 += _ATs++;
        p2 += _99 - _77;
        #elif 0
        p0 += _AKo;
        p1 += _KQs;
        p2 += _Q6s- _Q4s;
        p3 += _ATo++;
        #elif 1
        p0 += _AKo;
        p1 += _KQs;
        p2 += _Q6s- _Q4s;
        p3 += _ATo++;
        p4 += _TT-_77;
        #else
        p0 += _AKs;
        p1 += _QJs;
        p2 += _T9s;
        #endif

        run_driver(std::vector<frontend::range>{p0, p1});
        std::cout << std::string(100,'-') << std::endl;
        run_driver(std::vector<frontend::range>{p0, p1, p2});
        std::cout << std::string(100,'-') << std::endl;
        run_driver(std::vector<frontend::range>{p0, p1, p2, p3});
        std::cout << std::string(100,'-') << std::endl;
        #if 0
        run_driver(std::vector<frontend::range>{p0, p1, p2, p3,p4});
        std::cout << std::string(100,'-') << std::endl;
        #endif

        
}

void test1(){
        using namespace ps;
        using namespace ps::frontend;

        range p0;
        range p1;
        p0 += _AKo - _AQo;
        p0 += _TT++;
        p1 += _AKo - _AQo;
        p1 += _TT++;
        run_driver(std::vector<frontend::range>{p0, p1});
}

int main(){
        test0();
}
