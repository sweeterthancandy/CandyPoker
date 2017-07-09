#include "ps/calculator_detail.h"
#include "ps/calculator.h"
#include "ps/heads_up.h"
#include "ps/frontend.h"

#include <mutex>
#include <thread>
#include <condition_variable>

#include <boost/timer/timer.hpp>
#include <boost/mpl/size_t.hpp>

using namespace ps;
using namespace ps::detail;


template<class... Vecs, class F>
void visit_vector_combinations(Vecs... vecs, F f){
        detail::visit_exclusive_combinations<sizeof...(args)>(
                [&](auto... idx){
                }
                , detail::true_, 

}

int main(){
        std::vector<int> a = {1,1,1};
        std::vector<int> b = {1,2,4};
        std::vector<int> c = {1,3,9};

        visit_vector_combinations( a, b, c,
                [](auto i, auto j, auto k){
        }
}
