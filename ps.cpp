#include "ps/symbolic.h"
#include "ps/transforms.h"

#include <type_traits>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>


namespace{
        void run_driver(std::vector<ps::frontend::range> const& players)
        {

                using namespace ps;
                using namespace ps::frontend;

                symbolic_computation::handle star = std::make_shared<symbolic_range>( players );

                //star->print();

                symbolic_computation::transform_schedular sch;

                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          transforms::to_lowest_permutation() );
                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          transforms::remove_suit_perms() );
                {
                        boost::timer::auto_cpu_timer at;
                        sch.execute(star);
                }
                
                //star->print();

                calculation_cache cache;
        
                boost::timer::auto_cpu_timer at;
                auto ret{ star->calculate(cache) };

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
                PRINT(ret);
        }
} // anon
 
int main(){

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
        run_driver(std::vector<frontend::range>{p0, p1, p2});
        run_driver(std::vector<frontend::range>{p0, p1, p2, p3});
        #if 0
        run_driver(std::vector<frontend::range>{p0, p1, p2, p3,p4});
        #endif
        
}
