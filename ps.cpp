#include "ps/holdem/symbolic.h"
#include "ps/holdem/numeric.h"
#include "ps/holdem/transforms.h"

#include <boost/timer/timer.hpp>


namespace{
void run_driver(std::vector<ps::frontend::range> const& players)
{

        using namespace ps;
        using namespace ps::frontend;

        symbolic_computation::handle star = std::make_shared<symbolic_range>( players );
        star->print();

        ps::numeric::work_scheduler work{players.size()};

        symbolic_computation::transform_schedular sch;

        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::to_lowest_permutation() );
        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::remove_suit_perms() );
        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::work_generator(work) );
        sch.execute(star);

        ps::numeric::result_type ret{players.size()};
        {
                boost::timer::auto_cpu_timer at;
                ret = work.compute();
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

        range villian;

        p0 += _AA;

        p1 += _AKo;

        p2 += _22;

        run_driver(std::vector<frontend::range>{p0, p1});
        run_driver(std::vector<frontend::range>{p0, p1, p2});

        
}
