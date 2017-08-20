#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <bitset>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/detail/print.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/evaluator.h"
#include "ps/sim/holdem_class_strategy.h"
#include "ps/support/index_sequence.h"
#include "ps/support/config.h"


using namespace ps;

struct hasher{
        using hash_t = std::uintfast64_t;

        hash_t create(){ return 0; }
        hash_t create(rank_vector const& rv){
                auto hash = create();
                for(auto id : rv )
                        hash = append(hash, id);
                return hash;
        }
        hash_t append(hash_t hash, rank_id rank){
                auto idx = rank * 2;
                auto mask = ( hash & ( 0x3 << idx ) ) >> idx;
                switch(mask){
                case 0x0:
                        // set the idx'th bit
                        hash |= 0x1 << idx;
                        break;
                case 0x1:
                        // unset the idx'th bit
                        hash &= ~(0x1 << idx);
                        // set the (idx+1)'th bit
                        hash |= 0x1 << (idx+1);
                        break;
                case 0x2:
                        // set the idx'th bit
                        hash |= 0x1 << idx;
                        break;
                case 0x3:
                        // set the special part of mask for the fourth card
                        hash |= (rank + 1) << 0x1A;
                        break;
                default:
                        PS_UNREACHABLE();
                }
        }
};

struct item{
        size_t         mask;
        hasher::hash_t hash;
};


int main(){
        holdem_class_vector cv;
        cv.push_back("AA");
        cv.push_back("KK");
        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
        for( auto hv : players.get_hand_vectors()){
        }
        return result;
}
