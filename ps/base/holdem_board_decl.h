#ifndef PS_BASE_HOLDEM_BOARD_DECL_H
#define PS_BASE_HOLDEM_BOARD_DECL_H

namespace ps{

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
                size_t mask()const noexcept{ return mask_; }
                size_t rank_hash()const noexcept{ return rank_hash_; }
                size_t suit_hash()const noexcept{ return suit_hash_; }
                card_vector const& board()const noexcept{ return vec_; }
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
        auto begin()const noexcept{ return world_.begin(); }
        auto end()const noexcept{ return world_.end(); }

private:
        std::vector<layout> world_;
};

} // ps

#endif // PS_BASE_HOLDEM_BOARD_DECL_H
