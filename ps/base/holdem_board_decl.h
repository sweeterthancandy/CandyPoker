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
        
        struct lightweight_layout_singleton{

                lightweight_layout_singleton(size_t rank_mask, suit_id flush_suit, size_t flush_mask, std::array<ranking_t, 13 * 13 + 13> local_eval)
                        :flush_suit_(flush_suit),
                        flush_mask_(flush_mask),
                        rank_mask_(rank_mask),
                        local_eval_(local_eval)
                {
                        masks.add(rank_mask);
                }


                suit_id            flush_suit()const noexcept{ return flush_suit_; }
                size_t             flush_mask()const{ return flush_mask_; };

                ranking_t          no_flush_rank(card_id c0, card_id c1)const noexcept{
                        return local_eval_[ c0 * 13 + c1 ];
                }
                size_t single_rank_mask()const{
                        return rank_mask_;
                }
                suit_id flush_suit_{0};
                size_t flush_mask_{0};
                size_t rank_mask_;
                std::array<ranking_t, 13 * 13 + 13> local_eval_;
                mask_set masks;
        };

        struct lightweight_layout_aggregate{

                lightweight_layout_aggregate(size_t rank_mask, suit_id flush_suit, size_t flush_mask, std::array<ranking_t, 13 * 13 + 13> local_eval)
                        :flush_suit_(flush_suit),
                        flush_mask_(flush_mask),
                        local_eval_(local_eval)
                {
                        masks.add(rank_mask);
                }


                suit_id            flush_suit()const noexcept{ return flush_suit_; }
                size_t             flush_mask()const{ return flush_mask_; };


                ranking_t          no_flush_rank(card_id c0, card_id c1)const noexcept{
                        return local_eval_[ c0 * 13 + c1 ];
                }
                suit_id flush_suit_{0};
                size_t flush_mask_{0};
                std::array<ranking_t, 13 * 13 + 13> local_eval_;
                mask_set masks;
        };
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
                
                        card_vector flush_suit_board_;

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
                size_t             flush_mask()const{ return flush_mask_; };

                ranking_t          no_flush_rank(card_id c0, card_id c1)const noexcept{
                        return local_eval_[ c0 * 13 + c1 ];
                }

                template<class Type>
                Type make_lightweight()const{
                        return Type{mask(), flush_suit_, flush_mask_, local_eval_};
                }
        private:
                size_t mask_;
                card_vector vec_;
                rank_hasher::rank_hash_t rank_hash_{0};
                suit_hasher::suit_hash_t suit_hash_{0};
                suit_id flush_suit_{0};
                bool flush_possible_;
                size_t flush_mask_{0};
                friend struct super_duper_board_opt_idea;
                std::array<ranking_t, 13 * 13 + 13> local_eval_;
        };

        holdem_board_decl(){
                rank_hash_eval eval;
                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        world_.emplace_back(eval, *iter );
                }

                // mask -> index
                std::unordered_map<size_t, size_t> m;

                size_t singletons{0};
                size_t aggregates{0};


                for( auto const& l : world_ ){
                        if( l.flush_possible() ){
                                // singleton
                                weighted_singletons_.emplace_back( l.make_lightweight<lightweight_layout_singleton>() );
                                ++singletons;
                        } else {
                                // try to aggrefate
                                auto iter = m.find( l.rank_hash() );
                                if( iter == m.end()){
                                        // first one
                                        weighted_aggregates_.emplace_back( l.make_lightweight<lightweight_layout_aggregate>() );
                                        m[l.rank_hash()] = weighted_aggregates_.size() -1 ;
                                } else {
                                        weighted_aggregates_[iter->second].masks.add(l.mask());
                                }
                                ++aggregates;
                        }
                }

                PS_LOG(trace) << "singletons=" << singletons << ", aggregates=" << aggregates << " ( "
                         << (( aggregates * 100.0 ) / ( singletons + aggregates ));

                //std::cout << "max mask size = " << boost::max_element(  weighted_, [](auto const& l, auto const& r){ return l.masks.size() < r.masks.size(); })->masks.size() << "\n";
                profile_memory();


                #if 0
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
                #endif

        }
        auto begin()const noexcept{ return world_.begin(); }
        auto end()const noexcept{ return world_.end(); }

        auto const& weighted_aggregate_rng()const{ return weighted_aggregates_; }
        auto const& weighted_singleton_rng()const{ return weighted_singletons_; }

        void profile_memory(){
                auto world_gb    = world_.size() * sizeof(layout) * 1.0 / 1024 / 1024 / 1024;
                //auto weighted_gb = weighted_.size() * sizeof(layout) * 1.0 / 1024 / 1024 / 1024;

                std::cout << "world_gb => " << world_gb << "\n"; // __CandyPrint__(cxx-print-scalar,world_gb)
                //std::cout << "weighted_gb => " << weighted_gb << "\n"; // __CandyPrint__(cxx-print-scalar,weighted_gb)

        }
private:
        std::vector<layout> world_;
        std::vector<lightweight_layout_singleton> weighted_singletons_;
        std::vector<lightweight_layout_aggregate> weighted_aggregates_;
};

} // ps

#endif // PS_BASE_HOLDEM_BOARD_DECL_H
