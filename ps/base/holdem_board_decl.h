#ifndef PS_BASE_HOLDEM_BOARD_DECL_H
#define PS_BASE_HOLDEM_BOARD_DECL_H

#include "ps/base/hash.h"
#include "ps/base/board_combination_iterator.h"

namespace ps{

/*
        Idea here is that I'm not concerned about
        creating an object representing 5 cards,
        so that we create another object after adding
        2 more cards, ie, 

        case

        aabcd => only rank
        aabbc => only rank
        aaabb => ( AA => max{aaaAA, 

                  __________{7}
               __/           |
              /              |
      ______{6}            {6}  ...
     /   /   |  
    {5} {5} {5} ...

 */

/*
        This represents 5 cards, with hashes precomputed.
 */
struct hash_precompute{
        static constexpr const size_t min_depth{5};
        hash_precompute(card_vector _cards)
                :cards{std::move(_cards)}
        {
                //PRINT(cards.size());
                hash = card_hash_create_from_cards(cards);
                if( cards.size() > min_depth ){
                        for(size_t i=0;i!=cards.size();++i){
                                card_vector cv;
                                for(size_t j=0;j!=cards.size();++j){
                                        if(j!=i){
                                                cv.push_back(cards[j]);
                                        }
                                }
                                sub.emplace_back(std::move(cv));
                        }
                }
        }
        card_vector cards;
        card_hash_t hash;
        std::vector<hash_precompute> sub;
};

struct holdem_board_decl{
        struct layout{
                layout(card_vector vec)
                        :vec_{std::move(vec)}
                        ,mask_{vec_.mask()}
                        ,hash_{card_hash_create_from_cards(vec_)}
                        ,pre_{vec_}
                {}
                size_t mask()const{ return mask_; }
                size_t hash()const{ return hash_; }
                card_vector const& board()const{ return vec_; }
                hash_precompute const& pre()const{ return pre_; }
        private:
                card_vector vec_;
                size_t mask_;
                card_hash_t hash_{0};
                hash_precompute pre_;
        };

        holdem_board_decl(){
                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        world_.emplace_back( *iter );
                }
        }
        auto begin()const{ return world_.begin(); }
        auto end()const{ return world_.end(); }

private:
        std::vector<layout> world_;
};
#if 0
struct holdem_board_decl{
        struct layout{
                layout(card_vector vec)
                        :vec_{std::move(vec)}
                {
                        static suit_hasher sh;
                        static rank_hasher rh;
                                
                        rank_hash_ = rh.create();
                        suit_hash_ = sh.create();

                        for( auto id : vec_ ){
                                auto const& hand{ card_decl::get(id) };

                                rank_hash_ = rh.append(rank_hash_, hand.rank() );
                                suit_hash_ = sh.append(suit_hash_, hand.suit() );
                        }
                        mask_ = vec_.mask();
                }
                size_t mask()const{ return mask_; }
                size_t rank_hash()const{ return rank_hash_; }
                size_t suit_hash()const{ return suit_hash_; }
                card_vector const& board()const{ return vec_; }
        private:
                size_t mask_;
                card_vector vec_;
                size_t rank_hash_{0};
                size_t suit_hash_{0};
        };

        holdem_board_decl(){
                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        world_.emplace_back( *iter );
                }
        }
        auto begin()const{ return world_.begin(); }
        auto end()const{ return world_.end(); }

private:
        std::vector<layout> world_;
};
#endif

} // ps

#endif // PS_BASE_HOLDEM_BOARD_DECL_H
