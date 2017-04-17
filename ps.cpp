#include "ps/holdem/symbolic.h"
#include "ps/holdem/numeric.h"
#include "ps/holdem/transforms.h"

int main(){

        using namespace ps;
        using namespace ps::frontend;

        range hero;
        hero += _AKo;

        range villian;
        villian += _55;

        range other_villian;
        other_villian += _89s;

        symbolic_computation::handle star = std::make_shared<symbolic_range>( std::vector<frontend::range>{villian, hero, other_villian} );

        ps::numeric::work_scheduler work{3};

        symbolic_computation::transform_schedular sch;

        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::to_lowest_permutation() );
        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::remove_suit_perms() );
        sch.decl( symbolic_computation::transform_schedular::TransformKind_BottomUp,
                  transforms::work_generator(work) );
        sch.execute(star);


        PRINT(work.compute());
        
}
