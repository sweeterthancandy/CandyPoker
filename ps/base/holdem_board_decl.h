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
#include "ps/base/mask_set.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/rank_decl.h"
#include "ps/eval/rank_hash_eval.h"

#include <boost/iterator/indirect_iterator.hpp>

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
        enum{ Debug = false };
        struct layout{
                layout(rank_hash_eval const& eval, card_vector vec)
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

                        local_eval_.fill(-1);
                        for(size_t c0=0;c0!=13;++c0){
                                for(size_t c1=0;c1!=13;++c1){

                                        auto mask = rank_hash_;
                                        mask = rank_hasher::append(mask, c0);
                                        mask = rank_hasher::append(mask, c1);

                                        local_eval_[c0 * 13 + c1 ] = eval.rank_no_flush(mask);
                                }
                        }

                }
                size_t mask()const noexcept{ return mask_; }
                size_t rank_hash()const noexcept{ return rank_hash_; }
                size_t suit_hash()const noexcept{ return suit_hash_; }
                card_vector const& board()const noexcept{ return vec_; }

                bool               flush_possible()const noexcept{ return flush_possible_; }
                suit_id            flush_suit()const noexcept{ return flush_suit_; }
                card_vector const& flush_suit_board()const noexcept{ return flush_suit_board_; }
                size_t             flush_mask()const{ return flush_mask_; };

                ranking_t          no_flush_rank(card_id c0, card_id c1)const noexcept{
                        return local_eval_[ c0 * 13 + c1 ];
                }
        private:
                size_t mask_;
                card_vector vec_;
                rank_hasher::rank_hash_t rank_hash_{0};
                suit_hasher::suit_hash_t suit_hash_{0};
                card_vector flush_suit_board_;
                suit_id flush_suit_{0};
                bool flush_possible_;
                size_t flush_mask_{0};
                friend struct super_duper_board_opt_idea;
                std::array<ranking_t, 13 * 13 + 13> local_eval_;
        };
        struct weighted_layout{
                layout const* board;
                mask_set masks;
        };

        holdem_board_decl(){
                rank_hash_eval eval;
                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        world_.emplace_back(eval, *iter );
                }

                // mask -> index
                std::unordered_map<size_t, size_t> m;

                for( auto const& l : world_ ){
                        if( l.flush_possible() ){
                                weighted_.emplace_back( weighted_layout{&l} );
                                weighted_.back().masks.add( l.mask() );
                        } else {
                                auto iter = m.find( l.rank_hash() );
                                if( iter == m.end()){
                                        weighted_.emplace_back( weighted_layout{&l} );
                                        weighted_.back().masks.add( l.mask() );
                                        m[l.rank_hash()] = weighted_.size() -1 ;
                                } else {
                                        weighted_[iter->second].masks.add(l.mask());
                                }
                        }
                }

                if( Debug ){

                        std::map<mask_set, size_t> hist;
                        size_t masks_sigma = 0;
                        for(auto const& p : weighted_ ){
                                ++hist[p.masks];
                                ++masks_sigma;
                        }
                        std::cout << "hist.size() => " << hist.size() << "\n"; // __CandyPrint__(cxx-print-scalar,hist.size())
                        std::cout << "masks_sigma => " << masks_sigma << "\n"; // __CandyPrint__(cxx-print-scalar,masks_sigma)

                        // reorder them, so it's possible for branch prediction
                        boost::sort(  weighted_, [](auto const& l, auto const& r){
                                return l.masks.size() < r.masks.size();
                        });

                        size_t sigma = 0;
                        for(auto const& _ : weighted_ ){
                                sigma += _.masks.size();
                        }

                        std::cout << "world_.size() => " << world_.size() << ", "; // __CandyPrint__(cxx-print-scalar,world_.size())
                        std::cout << "weighted_.size() => " << weighted_.size() << "\n"; // __CandyPrint__(cxx-print-scalar,weighted_.size())
                        std::cout << "sigma => " << sigma << "\n"; // __CandyPrint__(cxx-print-scalar,sigma)
                }

        }
        auto begin()const noexcept{ return world_.begin(); }
        auto end()const noexcept{ return world_.end(); }

        auto const& weighted_rng()const{ return weighted_; }
private:
        std::vector<layout> world_;
        std::vector<weighted_layout> weighted_;
        
};

enum board_type{
        BOP_NoFlush,
        BOP_ThreeFlush = 3,
        BOP_FourFlush  = 4,
        BOP_FiveFlush  = 5,
};
struct board_subset{
        explicit board_subset(board_type type):type_{type}{}
        virtual ~board_subset()=default;
        board_type type()const{ return type_; }
private:
        board_type type_;
};
struct board_no_flush_subset : board_subset{
        struct atom{
                atom(mask_set masks_, std::array<ranking_t, 13 * 13 + 13> local_eval)
                        :masks(masks_),
                        local_eval_(local_eval)
                {}
                mask_set masks;
                std::array<ranking_t, 13 * 13 + 13> local_eval_;

                ranking_t          no_flush_rank(card_id c0, card_id c1)const noexcept{
                        return local_eval_[ c0 * 13 + c1 ];
                }
        };
        board_no_flush_subset():board_subset{BOP_NoFlush}{}
        auto begin()const{ return atoms_.begin(); }
        auto end()const{ return atoms_.end(); }
private:
        friend struct super_duper_board_opt_idea;
        std::vector<atom> atoms_;
};
struct board_flush_subset : board_subset{
        struct atom{
                atom(mask_set masks_, std::array<ranking_t, 13 * 13 + 13> local_eval, size_t flush_mask_, suit_id flush_suit_)
                        :masks(masks_),
                        local_eval_(local_eval),
                        flush_mask(flush_mask_),
                        flush_suit(flush_suit_)
                {}
                mask_set masks;
                std::array<ranking_t, 13 * 13 + 13> local_eval_;
                std::uint16_t flush_mask{0};
                suit_id flush_suit{0};
                
                ranking_t          no_flush_rank(card_id c0, card_id c1)const noexcept{
                        return local_eval_[ c0 * 13 + c1 ];
                }
        };

        explicit board_flush_subset(board_type type):board_subset{type}{}

        auto begin()const{ return atoms_.begin(); }
        auto end()const{ return atoms_.end(); }
private:
        friend struct super_duper_board_opt_idea;
        std::vector<atom> atoms_;
};


struct super_duper_board_opt_idea{
        super_duper_board_opt_idea(){
                boost::timer::auto_cpu_timer at(2, "super_duper_board_opt_idea took %w seconds\n");
                holdem_board_decl w;
                auto _0 = std::make_shared<board_no_flush_subset>();
                auto _3 = std::make_shared<board_flush_subset>(BOP_ThreeFlush);
                auto _4 = std::make_shared<board_flush_subset>(BOP_FourFlush);
                auto _5 = std::make_shared<board_flush_subset>(BOP_FiveFlush);
                for(auto const& weighted_pair : w.weighted_rng() ){

                        auto const& b = *weighted_pair.board;
                        auto rank_proto       = b.rank_hash();
                        auto const& flush_suit_board = b.flush_suit_board();
                        size_t fsbsz = flush_suit_board.size();
                        suit_id flush_suit    = b.flush_suit();
                        auto flush_mask = b.flush_mask();

                        switch(fsbsz){
                        case 0:
                                _0->atoms_.emplace_back(weighted_pair.masks, b.local_eval_);
                                break;
                        case 3:
                                _3->atoms_.emplace_back(weighted_pair.masks, b.local_eval_, flush_mask, flush_suit);
                                break;
                        case 4:
                                _4->atoms_.emplace_back(weighted_pair.masks, b.local_eval_, flush_mask, flush_suit);
                                break;
                        case 5:
                                _5->atoms_.emplace_back(weighted_pair.masks, b.local_eval_, flush_mask, flush_suit);
                                break;
                        }
                }
                subsets_.push_back(_0);
                subsets_.push_back(_3);
                subsets_.push_back(_4);
                subsets_.push_back(_5);
        }
        auto begin()const{ return boost::make_indirect_iterator(subsets_.begin()); }
        auto end()const{ return boost::make_indirect_iterator(subsets_.end()); }
private:
        std::vector<
                std::shared_ptr<board_subset>
        > subsets_;
};

} // ps

#endif // PS_BASE_HOLDEM_BOARD_DECL_H
