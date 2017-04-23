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

                star->print();

                symbolic_computation::transform_schedular sch;

                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          transforms::to_lowest_permutation() );
                sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                          transforms::remove_suit_perms() );
                sch.execute(star);
                
                star->print();

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

                const char* title_fmt{ "%2s %12s %12s %10s %10s %20s %10s\n" };
                const char* fmt{       "%2s %12s %12s %10s %10s %20s %10.4f\n" };
                std::cout << boost::format(title_fmt)
                        % "n" % "wins" % "draws2" % "draws3" % "sigma" % "equity" % "equity %";
                for(size_t i{0};i!=ret.size1();++i){
                        std::cout << boost::format(fmt)
                                % i % fmtc(ret(i,0)) % fmtc(ret(i,1)) % fmtc(ret(i,2)) 
                                % fmtc(ret(i,9)) % fmtc(ret(i,10)) 
                                % ( static_cast<double>(ret(i,10) ) / computation_equity_fixed_prec / ret(i,9) * 100 );
                }
        }
} // anon
 
int main(){

        using namespace ps;
        using namespace ps::frontend;

        range p0;
        range p1;
        range p2;
        range p3;

        range villian;


        #if 0
        p1 += _KQs;
        p0 += _QJs;
        p2 += _AKs;
        #else
        p0 += _AKs;
        p1 += _KQs;
        p2 += _QJs;
        #endif

        //run_driver(std::vector<frontend::range>{p0, p1});
        run_driver(std::vector<frontend::range>{p0, p1, p2});
        
}
