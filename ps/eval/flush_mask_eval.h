#ifndef PS_EVAL_FLUSH_MASK_EVAL_H
#define PS_EVAL_FLUSH_MASK_EVAL_H

#include "ps/eval/evaluator_6_card_map.h"
#include "ps/detail/popcount.h"

namespace ps{

/*
        If we have a set of flush cards, because we only have one of each 
        card, we can just lookup a 13-bit mask
 */
struct flush_mask_eval{
        //                      AKQJT98765432
        enum{ RankMaskUpper = 0b1111111000000 };

        template<class Array, class Eval>
        static void create_flush_mask_eval_inplace(Array& array, Eval const& eval){
                for(size_t mask = 0; mask <= RankMaskUpper ; ++mask ){
                        auto pc = detail::popcount(mask);
                        switch(pc){
                        case 5:
                        case 6:
                        case 7:
                                break;
                        default:
                                continue;
                        }

                        std::array<size_t, 7> ids;
                        size_t iter = 0;
                        for(size_t rank=0;rank!=13;++rank){
                                if( mask & (1ull << rank) ){
                                        ids[iter] = rank;
                                        ++iter;
                                }
                        }
                        PS_ASSERT( iter == pc, "iter = " << iter );

                        static auto one = 1ull;

                        switch(pc){
                                case 5:
                                {
                                        auto val = eval( ids[0],
                                                         ids[1],
                                                         ids[2],
                                                         ids[3],
                                                         ids[4] );
                                        auto mask = ( one << ids[0] | 
                                                      one << ids[1] |
                                                      one << ids[2] |
                                                      one << ids[3] |
                                                      one << ids[4] );
                                        array[mask] = val;
                                        break;
                                }
                                case 6:
                                {
                                        auto val = eval( ids[0],
                                                         ids[1],
                                                         ids[2],
                                                         ids[3],
                                                         ids[4],
                                                         ids[5] );
                                        auto mask = ( one << ids[0] | 
                                                      one << ids[1] |
                                                      one << ids[2] |
                                                      one << ids[3] |
                                                      one << ids[4] |
                                                      one << ids[5] );
                                        array[mask] = val;
                                        break;
                                }
                                case 7:
                                {
                                        auto val = eval( ids[0],
                                                         ids[1],
                                                         ids[2],
                                                         ids[3],
                                                         ids[4],
                                                         ids[5],
                                                         ids[6] );
                                        auto mask = ( one << ids[0] | 
                                                      one << ids[1] |
                                                      one << ids[2] |
                                                      one << ids[3] |
                                                      one << ids[4] |
                                                      one << ids[5] |
                                                      one << ids[6] );
                                        array[mask] = val;
                                        break;
                                }
                        }
                }
        }
        template<class Eval>
        static std::shared_ptr<flush_mask_eval> create(Eval const& eval)noexcept{
                auto result = std::make_shared<flush_mask_eval>();
                result->suit_map_.fill(static_cast<ranking_t>(-1));
                create_flush_mask_eval_inplace(result->suit_map_, eval);
                return result;
        }

        flush_mask_eval(){
                evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
                suit_map_.fill(static_cast<ranking_t>(-1));
                auto flush_wrap = [&](auto... c){
                        return e6cm_->rank(card_decl::make_id(0,c)...);
                };
                flush_mask_eval::create_flush_mask_eval_inplace(suit_map_, flush_wrap);
        }

        ranking_t operator()(size_t mask)const noexcept{
                return suit_map_[mask];
        }
private:
        std::array<ranking_t, RankMaskUpper + 1> suit_map_;
};





/*
        If we have a set of flush cards, because we only have one of each 
        card, we can just lookup a 13-bit mask
 */
struct no_flush_no_pair_mask{
        //                      AKQJT98765432
        enum{ RankMaskUpper = 0b1111111000000 };

        template<class Array, class Eval>
        static void create_flush_mask_eval_inplace(Array& array, Eval const& eval){
                for(size_t mask = 0; mask <= RankMaskUpper ; ++mask ){
                        auto pc = detail::popcount(mask);
                        switch(pc){
                        case 5:
                        case 6:
                        case 7:
                                break;
                        default:
                                continue;
                        }

                        std::array<size_t, 7> ids;
                        size_t iter = 0;
                        for(size_t rank=0;rank!=13;++rank){
                                if( mask & (1ull << rank) ){
                                        ids[iter] = rank;
                                        ++iter;
                                }
                        }
                        PS_ASSERT( iter == pc, "iter = " << iter );

                        static auto one = 1ull;

                        switch(pc){
                                case 5:
                                {
                                        auto val = eval( ids[0],
                                                         ids[1],
                                                         ids[2],
                                                         ids[3],
                                                         ids[4] );
                                        auto mask = ( one << ids[0] | 
                                                      one << ids[1] |
                                                      one << ids[2] |
                                                      one << ids[3] |
                                                      one << ids[4] );
                                        array[mask] = val;
                                        break;
                                }
                                case 6:
                                {
                                        auto val = eval( ids[0],
                                                         ids[1],
                                                         ids[2],
                                                         ids[3],
                                                         ids[4],
                                                         ids[5] );
                                        auto mask = ( one << ids[0] | 
                                                      one << ids[1] |
                                                      one << ids[2] |
                                                      one << ids[3] |
                                                      one << ids[4] |
                                                      one << ids[5] );
                                        array[mask] = val;
                                        break;
                                }
                                case 7:
                                {
                                        auto val = eval( ids[0],
                                                         ids[1],
                                                         ids[2],
                                                         ids[3],
                                                         ids[4],
                                                         ids[5],
                                                         ids[6] );
                                        auto mask = ( one << ids[0] | 
                                                      one << ids[1] |
                                                      one << ids[2] |
                                                      one << ids[3] |
                                                      one << ids[4] |
                                                      one << ids[5] |
                                                      one << ids[6] );
                                        array[mask] = val;
                                        break;
                                }
                        }
                }
        }
        template<class Eval>
        static std::shared_ptr<no_flush_no_pair_mask> create(Eval const& eval)noexcept{
                auto result = std::make_shared<no_flush_no_pair_mask>();
                result->map_.fill(static_cast<ranking_t>(-1));
                create_flush_mask_eval_inplace(result->map_, eval);
                return result;
        }

        no_flush_no_pair_mask(){
                evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
                map_.fill(static_cast<ranking_t>(-1));
                auto flush_wrap = [&](auto c0, auto c1, auto c2, auto... c_rest){
                        return e6cm_->rank(card_decl::make_id(0,c0),
                                           card_decl::make_id(0,c1),
                                           card_decl::make_id(0,c2),
                                           card_decl::make_id(1,c_rest)...);
                };
                no_flush_no_pair_mask::create_flush_mask_eval_inplace(map_, flush_wrap);
        }

        ranking_t operator()(size_t mask)const noexcept{
                return map_[mask];
        }
private:
        std::array<ranking_t, RankMaskUpper + 1> map_;
};

} // end namespace ps

#endif // PS_EVAL_FLUSH_MASK_EVAL_H
