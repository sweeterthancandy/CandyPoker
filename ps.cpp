#include "ps/calculator_detail.h"
#include "ps/calculator.h"
#include "ps/heads_up.h"
#include "ps/frontend.h"

#include <mutex>
#include <thread>
#include <condition_variable>

#include <boost/timer/timer.hpp>
#include <boost/mpl/size_t.hpp>

#include "ps/detail/visit_sequence.h"
#include "ps/detail/visit_combinations.h"

using namespace ps;
using namespace ps::detail;

int main(){
        std::vector<int> a = {1,1,1};
        std::vector<int> b = {1,2,4};
        std::vector<int> c = {1,3,9};

        visit_vector_combinations( 
                [](auto i, auto j, auto k){
                        PRINT_SEQ((i)(j)(k));
                },
                a, b, c
        );
}
