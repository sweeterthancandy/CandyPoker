
#include "ps/calculator_detail.h"
#include "ps/calculator.h"
#include "ps/heads_up.h"
#include "ps/frontend.h"
#include "ps/algorithm.h"

#include <mutex>
#include <thread>
#include <condition_variable>

#include <boost/timer/timer.hpp>
#include <boost/mpl/size_t.hpp>

#include "ps/detail/visit_sequence.h"
#include "ps/detail/visit_combinations.h"

using namespace ps;

struct holdem_hand_caster{
        template<class T>
        std::string operator()(T const& val)const{
                return holdem_hand_decl::get(val).to_string();
        }
};

template<class Con>
auto to_string(Con const& con){
        return detail::to_string(con, holdem_hand_caster{} );
}

int main(){
        boost::timer::auto_cpu_timer at;
        std::map<std::vector<ps::holdem_id>, int> world; 
        size_t sigma{0};
        detail::visit_exclusive_combinations<3>(
                [&](auto... args)
                {
                        std::vector<ps::holdem_id> from{ static_cast<ps::holdem_id>(args)...};
                        auto t{ permutate_for_the_better(
                                        std::vector<ps::holdem_id>{
                                                static_cast<ps::holdem_id>(args)...} ) };
                        auto to = std::get<1>(t);
                        #if 0
                        std::cout 
                                << to_string(from) << " => " 
                                << to_string(to) << " " 
                                << detail::to_string(std::get<0>(t)) << "\n";
                                #endif

                        ++world[std::get<1>(t)];
                        ++sigma;

                },
                detail::true_,
                holdem_hand_decl::max_id-1
        );

        PRINT_SEQ((sigma)(world.size()));

}
