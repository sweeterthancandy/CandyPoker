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
#ifndef PS_BASE_RANK_BOARD_COMBINATION_ITERATOR_H
#define PS_BASE_RANK_BOARD_COMBINATION_ITERATOR_H

#include "ps/base/cards.h"

namespace ps{

struct rank_board_combination_iterator{
        using implementation_t = basic_index_iterator<
                int, ordered_policy, rank_vector
        >;
        // construct psuedo end iterator
        rank_board_combination_iterator():end_flag_{true}{}
        explicit rank_board_combination_iterator(size_t n)
                : end_flag_{false}, impl_{n, 13}
        {
                skip_to_valid_();
        }
        rank_vector const& operator*()const{ return *impl_; }

        rank_board_combination_iterator& operator++(){
                BOOST_ASSERT( ! end_flag_ );
                BOOST_ASSERT( impl_ != end_ );
                ++impl_;
                skip_to_valid_();
                return *this;
        }

        bool operator==(rank_board_combination_iterator const& that)const{
                return this->end_flag_ && that.end_flag_;
        }
        bool operator!=(rank_board_combination_iterator const& that)const{
                return ! ( *this == that);
        }
private:
        void skip_to_valid_(){
                for(;impl_!=end_;++impl_){
                        auto const& b = *impl_;
                        // first check we don't have more than 4 of each card
                        std::array<int, 13> aux = {0};
                        for(size_t i=0;i!=7;++i){
                                ++aux[b[i]];
                        }
                        bool not_possible = [&](){
                                for(size_t i=0;i!=aux.size();++i){
                                        if( aux[i] > 4 )
                                                return true;
                                }
                                return false;
                        }();
                        if( ! not_possible )
                                return;
                }
                if( impl_ == end_ )
                        end_flag_ = true;
        }
        bool end_flag_{true};
        implementation_t impl_;
        implementation_t end_;
};

} // end namespace ps

#endif // PS_BASE_RANK_BOARD_COMBINATION_ITERATOR_H
