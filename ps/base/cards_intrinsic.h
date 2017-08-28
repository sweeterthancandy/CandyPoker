#ifndef PS_BASE_CARD_INTRINSIC_H
#define PS_BASE_CARD_INTRINSIC_H

#include "ps/base/cards_fwd.h"

namespace ps{

        inline card_id card_suit_from_id(card_id id){
                return id & 0x3;
        }
        inline card_id card_rank_from_id(card_id id){
                return id >> 2;
        }
        inline size_t card_mask(card_id id){
                return static_cast<size_t>(1) << id;
        }
        // TODO 
        #if 0
        inline size_t holdem_hand_first(holdem_id id){
        }
        inline size_t holdem_hand_second(holdem_id id){
        }
        inline size_t holdem_hand_mask(holdem_id id){
        }
        #endif

} // ps

#endif // PS_BASE_CARD_INTRINSIC_H
