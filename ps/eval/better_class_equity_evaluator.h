#ifndef PS_EVAL_BETTER_CLASS_EQUITY_EVALUATOR_H
#define PS_EVAL_BETTER_CLASS_EQUITY_EVALUATOR_H

#include "ps/base/generate.h"
#include "ps/base/hash.h"
#include "ps/eval/rank_decl.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/detail/dispatch.h"

#include <memory>

namespace ps{

struct hash_ranker{
        hash_ranker(){
                rank_map_ .resize(rank_hash_max(7));
                flush_map_.resize(rank_hash_max(7));
        }
        void rank_commit(rank_hash_t hash, ranking_t r)noexcept{
                rank_map_[hash] = r;
        }
        void flush_commit(rank_hash_t hash, ranking_t r)noexcept{
                flush_map_[hash] = r;
        }
        ranking_t rank_eval(rank_hash_t hash)const noexcept{
                return rank_map_[hash];
        }
        ranking_t flush_eval(rank_hash_t hash)const noexcept{
                return flush_map_[hash];
        }
        void display(){
                for(size_t i=0;i < rank_map_.size();++i){
                        if( flush_map_[i] != 0 )
                                PRINT_SEQ((i)(flush_map_[i]));
                }
        }
private:
        std::vector<ranking_t> flush_map_;
        std::vector<ranking_t> rank_map_;
};

void hash_ranker_gen_5(hash_ranker& hr){
        struct hash_ranker_maker_detail{
                hash_ranker_maker_detail(hash_ranker* ptr):ptr_{ptr}{}
                void begin(std::string const&){}
                void end(){}
                void next( bool f, rank_id a, rank_id b, rank_id c, rank_id d, rank_id e){
                        auto hash = rank_hash_create(a,b,c,d,e);
                        if( f )
                                ptr_->flush_commit(hash, order_);
                        else
                                ptr_->rank_commit(hash, order_);
                        ++order_;
                }
                hash_ranker* ptr_;
                size_t order_{1};
        };
        hash_ranker_maker_detail aux(&hr);
        generate(aux);
}

void hash_ranker_gen_rank_n(hash_ranker& hr, size_t n){
        // need to do all this fancy work, because I want to 
        // go thought every permutation
        //      abcdefg,
        // where a <= b <= ..., a,b,c...\in{2,3,4,5,...,K,A},
        // but also that no more than 4 or each
        //
        using iter_t = basic_index_iterator<
                int, ordered_policy, rank_vector
        >;
        for(iter_t iter(n,13),end;iter!=end;++iter){
                [&](){
                        auto const& b = *iter;
                        std::array<int, 13> aux = {0};
                        for(size_t i=0;i!=b.size();++i){
                                ++aux[b[i]];
                        }
                        for(size_t i=0;i!=aux.size();++i){
                                if( aux[i] > 4 )
                                        return;
                        }

                        std::vector<ranking_t> rankings;
                        for(size_t i=0;i!=b.size();++i){
                                rank_vector rv;
                                for(size_t j=0;j!=b.size();++j){
                                        if(j!=i){
                                                rv.push_back(b[j]);
                                        }
                                }
                                rankings.push_back( hr.rank_eval( rank_hash_create(rv) ) );
                        }
                        auto lowest = * std::min_element(rankings.begin(), rankings.end());
                        hr.rank_commit( rank_hash_create(b), lowest);
                }();
        }
}





struct evaluator_5_card_hash{
        evaluator_5_card_hash(){
                hash_ranker_gen_5(impl_);
                hash_ranker_gen_rank_n(impl_, 6);
                hash_ranker_gen_rank_n(impl_, 7);
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e)const noexcept{
                auto hash = card_hash_create_from_cards(a,b,c,d,e);
                if( suit_hash_has_flush(card_hash_get_suit(hash) ) )
                        return impl_.flush_eval(card_hash_get_rank(hash));
                return impl_.rank_eval(hash);
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f)const noexcept{
                auto hash = card_hash_create_from_cards(a,b,c,d,e,f);
                if( ! suit_hash_has_flush(card_hash_get_suit(hash) ) ){
                        return impl_.rank_eval(hash);
                }
                std::array<ranking_t, 6> aux { 
                        rank(  b,c,d,e,f),
                        rank(a,  c,d,e,f),
                        rank(a,b,  d,e,f),
                        rank(a,b,c,  e,f),
                        rank(a,b,c,d,  f),
                        rank(a,b,c,d,e  )
                };
                return * std::min_element(aux.begin(), aux.end());
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f, card_id g)const noexcept{
                auto hash = card_hash_create_from_cards(a,b,c,d,e,f,g);
                if( ! suit_hash_has_flush(card_hash_get_suit(hash) ) ){
                        return impl_.rank_eval(hash);
                }
                std::array<ranking_t, 7> aux = {
                        rank(  b,c,d,e,f,g),
                        rank(a,  c,d,e,f,g),
                        rank(a,b,  d,e,f,g),
                        rank(a,b,c,  e,f,g),
                        rank(a,b,c,d,  f,g),
                        rank(a,b,c,d,e,  g),
                        rank(a,b,c,d,e,f  )
                };
                //PRINT( ::detail::to_string(aux) );
                return * std::min_element(aux.begin(), aux.end());
        }
        //mutable std::atomic_int miss{0};
        //mutable std::atomic_int hit{0};
        ranking_t rank_pre_7(hash_precompute const& pre, card_id x, card_id y)const noexcept{
                auto hash = card_hash_append_2(pre.hash, x, y);
                if( ! suit_hash_has_flush(card_hash_get_suit(hash) ) ){
                        //++hit;
                        return impl_.rank_eval(card_hash_get_rank(hash));
                }
                //++miss;
                
                // This case should happen 3% of the time
                //
                // Trying sub-caches gave worse performance
                auto a = pre.cards[0];
                auto b = pre.cards[1];
                auto c = pre.cards[2];
                auto d = pre.cards[3];
                auto e = pre.cards[4];
                auto f = x;
                auto g = y;
                std::array<ranking_t, 7> aux = {
                        rank(  b,c,d,e,f,g),
                        rank(a,  c,d,e,f,g),
                        rank(a,b,  d,e,f,g),
                        rank(a,b,c,  e,f,g),
                        rank(a,b,c,d,  f,g),
                        rank(a,b,c,d,e,  g),
                        rank(a,b,c,d,e,f  )
                };
                return * std::min_element(aux.begin(), aux.end());
        }
private:
        hash_ranker impl_;
};


struct better_class_equity_evaluator : class_equity_evaluator{
        std::shared_ptr<equity_breakdown> evaluate(holdem_class_vector const& cv)const{


                if( class_cache_ ){
                        auto ptr = class_cache_->try_lookup_perm(cv);
                        if( ptr )
                                return ptr;
                }
                
                auto t = cv.to_standard_form();

                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(cv.size());
                for( auto hvt : std::get<1>(t).to_standard_form_hands()){
                        auto const& perm = std::get<0>(hvt);
                        auto const& hv   = std::get<1>(hvt);
                        auto hv_mask = hv.mask();

                        if( cache_ ){
                                auto ptr = cache_->try_lookup_perm(hv);
                                if( ptr ){
                                        result->append_matrix(*ptr, perm );
                                        continue;
                                }
                        }

                                
                        // put this here
                        std::vector<ranking_t> ranked(hv.size());

                        // cache stuff

                        std::vector<card_id> hv_first(hv.size());
                        std::vector<card_id> hv_second(hv.size());
                        std::vector<rank_id> hv_first_rank(hv.size());
                        std::vector<rank_id> hv_second_rank(hv.size());
                        std::vector<suit_id> hv_first_suit(hv.size());
                        std::vector<suit_id> hv_second_suit(hv.size());
                                
                        for(size_t i=0;i!=hv.size();++i){
                                auto const& hand{holdem_hand_decl::get(hv[i])};

                                hv_first[i]       = hand.first().id();
                                hv_first_rank[i]  = hand.first().rank().id();
                                hv_first_suit[i]  = hand.first().suit().id();
                                hv_second[i]      = hand.second().id();
                                hv_second_rank[i] = hand.second().rank().id();
                                hv_second_suit[i] = hand.second().suit().id();
                        }

                        auto sub = std::make_shared<equity_breakdown_matrix_aggregator>(cv.size());
                        size_t board_count = 0;
                        for(auto const& b : w ){
                                        
                                bool cond = (b.mask() & hv_mask ) == 0;
                                if(!cond){
                                        continue;
                                }
                                ++board_count;
                                        
                                for(size_t i=0;i!=hv.size();++i){
                                        
                                        ranked[i] = ev.rank_pre_7( b.pre(), hv_first[i], hv_second[i]);

                                }
                                detail::dispatch_ranked_vector{}(*sub, ranked);

                        }

                        if( cache_ ){
                                cache_->lock();
                                cache_->commit( hv, *sub);
                                cache_->unlock();
                        }

                        result->append_matrix(*sub, perm );
                }
                        
                if( class_cache_ ){
                        class_cache_->lock();
                        class_cache_->commit( std::get<1>(t), *result);
                        class_cache_->unlock();
                }
                
                return std::make_shared<equity_breakdown_matrix>(*result, std::get<0>(t));;
        }
        void inject_class_cache(std::shared_ptr<holdem_class_eval_cache> ptr)override{
                class_cache_ = ptr;
        }
        void inject_cache(std::shared_ptr<holdem_eval_cache> ptr)override{
                cache_ = ptr;
        }
private:
        evaluator_5_card_hash ev;
        holdem_board_decl w;
        std::shared_ptr<holdem_eval_cache>  cache_;
        std::shared_ptr<holdem_class_eval_cache>  class_cache_;
};

} // ps

#endif // PS_EVAL_BETTER_CLASS_EQUITY_EVALUATOR_H
