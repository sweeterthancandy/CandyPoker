#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/detail/print.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/evaluator.h"
#include "ps/sim/holdem_class_strategy.h"


/*
                Strategy in the form 
                        vpip/fold
 */
#include <boost/range/algorithm.hpp>

/*
        Need to,

                iterate over boards
                        abcde  where a < b < c < d < e ( think strict lower triangle )
                iterator over hands
                        ab  where a <= b               ( think lower triable )


 */



using namespace ps;


struct evaluator_7_card_map : evaluator
{
        evaluator_7_card_map(){
                impl_ = &evaluator_factory::get("5_card_map");

                for(size_t i=0;i!=52;++i){
                        card_rank_device_[i] = card_decl::get(i).rank().id();
                }

                //for( index_sequence iter(7,52)



        }
        ranking_t rank(long a, long b, long c, long d, long e)const override{
                return impl_->rank(a,b,c,d,e);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
                return impl_->rank(a,b,c,d,e,f);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{
                return impl_->rank(a,b,c,d,e,f,g);
        }
private:
        auto make_hash_(long a, long b, long c, long d, long e, long f, long g)const{
                size_t hash = 0;

        } 
        evaluator* impl_;
        std::array<size_t, 52> card_rank_device_;
        std::vector<ranking_t> card_map_7_;
};

int main(){
        //for( holdem_class_iterator iter(2),end;iter!=end;++iter){
                //PRINT( *iter );
        //}
        //for( holdem_hand_iterator iter(2),end;iter!=end;++iter){
                //PRINT( *iter );
        //}
}
