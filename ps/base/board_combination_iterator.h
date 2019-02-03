/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_BASE_BOARD_COMBINATION_ITERATOR_H
#define PS_BASE_BOARD_COMBINATION_ITERATOR_H

#include "ps/base/cards.h"

namespace ps{

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
