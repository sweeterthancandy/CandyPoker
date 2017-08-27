#ifndef PS_BASE_BOARD_COMBINATION_ITERATOR_H
#define PS_BASE_BOARD_COMBINATION_ITERATOR_H

#include "ps/base/card_vector.h"

namespace ps{

/*
        This is for iterating over over boards in the form

                A B C D E
        where
                A < B < C < D < E
 */
struct board_combination_iterator{
        // construct psuedo end iterator
        board_combination_iterator():end_flag_{true}{}

        explicit board_combination_iterator(size_t n, std::vector<card_id> removed = std::vector<card_id>{});
        auto const& operator*()const{ return board_; }
        board_combination_iterator& operator++();

        bool operator==(board_combination_iterator const& that)const{
                return this->end_flag_ && that.end_flag_;
        }
        bool operator!=(board_combination_iterator const& that)const{
                return ! ( *this == that);
        }
private:
        // cards not to be run out on board
        card_vector removed_;
        // mapping m : c -> c, for next, ie when there are non
        // removed we have m = identity map
        std::vector<card_id> next_;
        // in memory rep
        card_vector board_;
        // size of board
        size_t n_;
        // flag to indicate at end
        bool end_flag_ = false;
        // last card, 0 when there are non removed
        card_id last_;
};

} // ps

#endif // PS_BASE_BOARD_COMBINATION_ITERATOR_H
