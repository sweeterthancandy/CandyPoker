
#include "ps/symbolic.h"
#include "ps/transforms.h"

#include <type_traits>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>

using namespace ps;

namespace{
        void run(std::vector<ps::frontend::range> const& players)
        {
                boost::timer::auto_cpu_timer at("driver took %w seconds\n");

                using namespace ps;
                using namespace ps::frontend;
                using namespace ps::transforms;

                symbolic_computation::handle star = std::make_shared<symbolic_range>( players );

                //star->print();

                symbolic_computation::transform_schedular sch;
                calculation_context ctx;


                sch.decl<permutate_for_the_better>();
                sch.decl<remove_suit_perms>();
                sch.decl<consolidate_dup_prim>();
                sch.decl<calc_primitive>(ctx);

                {
                        boost::timer::auto_cpu_timer at("tranforms took %w seconds\n");
                        sch.execute(star);
                }
                //star->print();

                
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
        int pokerstove_driver(int argc, char** argv){
                using namespace ps::frontend;
                std::vector<frontend::range> players;
                for(int i=1; i!= argc; ++i){
                        players.emplace_back( frontend::parse(argv[i]) );
                }
                run(players);
                return 0;
        }
} // anon

int main(int argc, char** argv){
        return pokerstove_driver(argc, argv);
}
