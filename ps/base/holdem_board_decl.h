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
#ifndef PS_BASE_HOLDEM_BOARD_DECL_H
#define PS_BASE_HOLDEM_BOARD_DECL_H

#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/board_combination_iterator.h"

namespace ps{

/*
        The idea is we pre-compute as much as possible

        My original implementaion was slow because I do 
                if( flush possible )
                        slow eval
                else
                        array lookup

        However, most times only a 5 card flush is possible, so we have
                
                if 7 card flush
                        super rare case, should contiube little time to
                        profile
                if 6 card flush
                        

 */
struct holdem_board_decl{
        struct layout{
                layout(card_vector vec)
                        :vec_{std::move(vec)}
                {
                        PS_ASSERT( vec_.size() == 5 , "false");
                                
                        rank_hash_ = rank_hasher::create();
                        suit_hash_ = suit_hasher::create();

                        std::array<size_t, 4> suit_hist = {0,0,0,0};

                        for( auto id : vec_ ){
                                auto const& hand{ card_decl::get(id) };

                                rank_hash_ = rank_hasher::append(rank_hash_, hand.rank() );
                                suit_hash_ = suit_hasher::append(suit_hash_, hand.suit() );

                                ++suit_hist[card_suit_from_id(id)];
                        }
                        mask_ = vec_.mask();
                        PS_ASSERT( __builtin_popcountll(mask_) == 5, "__builtin_popcountll(mask_) = " <<__builtin_popcountll(mask_) ); 

                        // do we have 3,4,5 of any suit?
                        // only one suit can have this
                        for(size_t sid =0;sid!=4;++sid){
                                if( suit_hist[sid] < 3 )
                                        continue;
                                // only possible for one suit
                                flush_suit_ = sid;
                                for( auto id : vec_ ){
                                        if( card_suit_from_id(id) != sid )
                                                continue;
                                        flush_suit_board_.push_back(id);
                                        flush_mask_ |= 1ull << card_rank_from_id(id);
                                }
                        }
                        flush_possible_ = ( flush_suit_board_.size() != 0);


                }
                size_t mask()const noexcept{ return mask_; }
                size_t rank_hash()const noexcept{ return rank_hash_; }
                size_t suit_hash()const noexcept{ return suit_hash_; }
                card_vector const& board()const noexcept{ return vec_; }

                bool               flush_possible()const noexcept{ return flush_possible_; }
                suit_id            flush_suit()const noexcept{ return flush_suit_; }
                card_vector const& flush_suit_board()const noexcept{ return flush_suit_board_; }
                size_t             flush_mask()const{ return flush_mask_; };
        private:
                size_t mask_;
                card_vector vec_;
                rank_hasher::rank_hash_t rank_hash_{0};
                suit_hasher::suit_hash_t suit_hash_{0};
                card_vector flush_suit_board_;
                suit_id flush_suit_{0};
                bool flush_possible_;
                size_t flush_mask_{0};
        };
        struct weighted_layout{
                size_t        weight;
                layout const* board;
        };

        holdem_board_decl(){
                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        world_.emplace_back( *iter );
                }

                // mask -> index
                std::unordered_map<size_t, size_t> m;

                for( auto const& l : world_ ){
                        if( l.flush_possible() ){
                                weighted_.emplace_back( weighted_layout{1, &l} );
                        } else {
                                auto iter = m.find( l.rank_hash() );
                                if( iter == m.end()){
                                        weighted_.emplace_back( weighted_layout{1, &l} );
                                        m[l.rank_hash()] = weighted_.size() -1 ;
                                } else {
                                        ++weighted_[iter->second].weight;
                                }
                        }
                }
                std::cout << "world_.size() => " << world_.size() << ", "; // __CandyPrint__(cxx-print-scalar,world_.size())
                std::cout << "weighted_.size() => " << weighted_.size() << "\n"; // __CandyPrint__(cxx-print-scalar,weighted_.size())
        }
        auto begin()const noexcept{ return world_.begin(); }
        auto end()const noexcept{ return world_.end(); }

        auto const& weighted_rng()const{ return weighted_; }
private:
        std::vector<layout> world_;
        std::vector<weighted_layout> weighted_;
        
};

} // ps

#endif // PS_BASE_HOLDEM_BOARD_DECL_H
