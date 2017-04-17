#include "ps/holdem/symbolic.h"
#include "ps/holdem/numeric.h"
#include "ps/holdem/transforms.h"

#include <boost/timer/timer.hpp>


void eval_2(ps::frontend::range const& p0,
            ps::frontend::range const& p1)
{

        using namespace ps;
        using namespace ps::frontend;

        symbolic_computation::handle star = std::make_shared<symbolic_range>( std::vector<frontend::range>{p0, p1} );

        ps::numeric::work_scheduler work{2};

        symbolic_computation::transform_schedular sch;

        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::to_lowest_permutation() );
        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::remove_suit_perms() );
        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::work_generator(work) );
        sch.execute(star);

        ps::numeric::matrix_type ret;
        {
                boost::timer::auto_cpu_timer at;
                ret = work.compute();
        }
        

        PRINT(ret);
}

int main(){

        using namespace ps;
        using namespace ps::frontend;

        range p0;
        range p1;

        range villian;

        p0 += _AA-_TT;

        p1 += _77;
        p1 += _67o;

        eval_2(p0, p1);

        
}
