#ifndef PS_EVAL_RANK_HASH_EVAL_H
#define PS_EVAL_RANK_HASH_EVAL_H

#include "ps/eval/flush_mask_eval.h"
#include "ps/eval/evaluator_6_card_map.h"

namespace ps{

struct rank_hash_eval
{
        rank_hash_eval(){
                evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
                card_map_7_.resize(rank_hasher::max()+1);
                auto rank_wrap = [&](auto c0, auto c1, auto c2, auto c4, auto... c_rest){
                        return e6cm_->rank( card_decl::make_id(0,c0),
                                            card_decl::make_id(0,c1),
                                            card_decl::make_id(0,c2),
                                            card_decl::make_id(0,c4),
                                            card_decl::make_id(1,c_rest)...);
                };
                rank_hasher::create_rank_hash_lookup_inplace(card_map_7_, rank_wrap);

                suit_map_.fill(static_cast<ranking_t>(-1));
                auto flush_wrap = [&](auto... c){
                        return e6cm_->rank(card_decl::make_id(0,c)...);
                };
                flush_mask_eval::create_flush_mask_eval_inplace(suit_map_, flush_wrap);
        }
        ranking_t rank_no_flush(size_t rank_hash)const noexcept{
                return card_map_7_[rank_hash];
        }
        ranking_t rank_only_flush(size_t flush_mask)const noexcept{
                return suit_map_[flush_mask];
        }
        ranking_t rank_flush(size_t rank_hash, size_t flush_mask)const noexcept{
                auto rr = card_map_7_[rank_hash];
                auto fr = suit_map_[flush_mask];
                return std::min<ranking_t>(rr, fr);
        }
private:
        std::vector<ranking_t> card_map_7_;
        std::array<ranking_t, flush_mask_eval::RankMaskUpper + 1> suit_map_;
};






struct rank_hash_hash_eval
{
        rank_hash_hash_eval(){
                for(rank_board_combination_iterator iter(7),end;iter!=end;++iter){
                        auto const& b = *iter;
                        auto hash = rank_hasher::create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                        auto val = e6cm_->rank( card_decl::make_id(0,b[0]),
                                                card_decl::make_id(0,b[1]),
                                                card_decl::make_id(0,b[2]),
                                                card_decl::make_id(0,b[3]),
                                                card_decl::make_id(1,b[4]),
                                                card_decl::make_id(1,b[5]),
                                                card_decl::make_id(1,b[6]) );

                        card_map_7_[hash] = val;
                }
                size_t bc = card_map_7_.bucket_count();
                std::cout << "bc => " << bc << "\n"; // __CandyPrint__(cxx-print-scalar,bc)
        }
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const noexcept{
                if( suit_hasher::has_flush_unsafe(suit_hash) ){
                        return e6cm_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                return card_map_7_.find(rank_hash)->second;
        }
        #if 0
        ranking_t rank_mask(size_t mask)const noexcept{
                auto iter = mask_map_.find(mask);
                if( iter == mask_map_.end())
                        return 0;
                return iter->second;
        }
        #endif
private:
        evaluator_6_card_map* e6cm_{evaluator_6_card_map::instance()};
        std::unordered_map<rank_hasher::rank_hash_t, ranking_t> card_map_7_;
};

#if 0
struct SKPokerEvalWrap
{
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const noexcept{
                auto remap = [](auto cid){
                        auto rank =      card_rank_from_id(cid);
                        auto suit =      card_suit_from_id(cid);
                        return rank * 4 + suit;
                };
                #if 0
                return SevenEval::GetRank<true>(remap(cv[0]),
                                                remap(cv[1]),
                                                remap(cv[2]),
                                                remap(cv[3]),
                                                remap(cv[4]),
                                                remap(a),
                                                remap(b));
                #else
                return SevenEval::GetRank<true>((cv[0]),
                                                (cv[1]),
                                                (cv[2]),
                                                (cv[3]),
                                                (cv[4]),
                                                (a),
                                                (b));
                #endif
        }
};
#endif




} // end namespace ps

#endif // PS_EVAL_RANK_HASH_EVAL_H
