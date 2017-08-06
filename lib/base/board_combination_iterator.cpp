#include "ps/base/board_combination_iterator.h"
#include "ps/base/cards.h"

#include <boost/range/algorithm.hpp>


namespace ps{

board_combination_iterator::board_combination_iterator(size_t n, std::vector<card_id> removed)
        : removed_{std::move(removed)}
        , n_{n}
{
        assert( n != 0 && "precondition failed");
        assert( n < 52 && "precondition failed");
        boost::sort(removed_);
        next_.assign(52,-1);
        for(card_id id{52};id!=0;){
                --id;

                if( boost::binary_search(removed_, id) ){
                        // removed card, ignore this mapping
                        continue;
                }

                if( id == 0 ){
                        // found last first case
                        last_ = id;
                        break;
                }
                card_id cand(id-1);
                // while card is in removed_, decrement
                for(; cand != 0 && boost::binary_search(removed_, cand);)--cand;
                if( boost::binary_search(removed_, cand) ){
                        // found last second case
                        last_ = id;
                        break;
                }
                next_[id] = cand;
        }
        PRINT(detail::to_string(removed_));
        PRINT(detail::to_string(next_));
        
        for(card_id id{52};id!=0 && board_.size() < n_;){
                --id;

                if( boost::binary_search(removed_, id) )
                        continue;
                board_.emplace_back(id);
        }

        if( board_.size() != n_ )
                BOOST_THROW_EXCEPTION(std::domain_error("unable to construct board of size n"));

        PRINT(last_);

}
board_combination_iterator& board_combination_iterator::operator++(){
        size_t cursor = n_ - 1;
        for(;;){

                // First see if we can't decrement the board
                // at the cursor
                if( cursor == n_ -1 ){
                        if( next_[board_[cursor]] == -1 ){
                                // case XXXX0
                                --cursor;
                                continue;
                        }
                } else {
                        if( next_[board_[cursor]] == board_[cursor+1] ){
                                // case XXX10
                                --cursor;
                                continue;
                        }
                }
                break;
        }
        if( cursor == -1 ){
                end_flag_ = true;
                return *this;
        }
        board_[cursor] = next_[board_[cursor]];
        ++cursor;
        for(;cursor != n_;++cursor){
                board_[cursor] = next_[board_[cursor-1]];
        }
        return *this;
}
} // ps
