#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <bitset>
#include <cstdint>
#include <future>
#include <boost/format.hpp>
#include <boost/timer/timer.hpp>
#include "ps/base/cards.h"
#include "ps/detail/print.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/evaluator.h"
#include "ps/eval/evaluator_7_card_map.h"
#include "ps/eval/evaluator_5_card_map.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/sim/holdem_class_strategy.h"
#include "ps/support/index_sequence.h"
#include "ps/support/config.h"

#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/detail/dispatch.h"

#include <boost/range/algorithm.hpp>


using namespace ps;




namespace working{

/*
        Idea here is that I'm not concerned about
        creating an object representing 5 cards,
        so that we create another object after adding
        2 more cards, ie, 

        case

        aabcd => only rank
        aabbc => only rank
        aaabb => ( AA => max{aaaAA, 

 */
struct card_chain{
};

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


        using suit_hash_t = std::uint32_t;

        suit_hash_t suit_hash_create(){
                return static_cast<suit_hash_t>(1);
        }
        suit_hash_t suit_hash_append(suit_hash_t hash, rank_id rank)noexcept{
                static constexpr const std::array<suit_id,4> suit_map = { 2,3,5,7 };
                return hash * suit_map[rank];
        }

        template<class... Args>
        suit_hash_t suit_hash_create_from_cards(Args... args){
                auto hash = suit_hash_create();
                int dummy[] = {0,  (hash = suit_hash_append(hash, card_suit_from_id(args)),0)...};
                return hash;
        }
        bool suit_hash_has_flush(suit_hash_t hash){
                if( hash == 0 )
                        return false;
                return 
                    ((hash % (2*2*2*2*2))*
                     (hash % (3*3*3*3*3))*
                     (hash % (5*5*5*5*5))*
                     (hash % (7*7*7*7*7))) == 0;
        }

        using rank_hash_t = std::uint32_t;
        
        /*
                  +----+--+--+--+--+--+--+--+--+--+--+--+--+--+
                  |card|A |K |Q |J |T |9 |8 |7 |6 |5 |4 |3 |2 |
                  +----+--+--+--+--+--+--+--+--+--+--+--+--+--+
                  |yyyy|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|xx|
                  +----+--+--+--+--+--+--+--+--+--+--+--+--+--+
                  |  1A|18|16|14|12|10| E| C| A| 8| 6| 4| 2| 0|
                  +----+--+--+--+--+--+--+--+--+--+--+--+--+--+

                  yyyy ~ value of rank with 4 cards, zero 
                         when there warn't 4 cards

                  xx   ~ bit mask to non-injective mapping for
                         number of cards, 


                                   n | bits
                                   --+-----
                                   0 | 00
                                   1 | 01
                                   2 | 10
                                   3 | 11
                                   4 | 11
        */
        rank_hash_t rank_hash_append(rank_hash_t hash, rank_id rank)noexcept{
                auto idx = rank * 2;
                auto mask = ( hash & ( 0x3 << idx ) ) >> idx;
                switch(mask){
                case 0x0:
                        // set the idx'th bit
                        hash |= 0x1 << idx;
                        break;
                case 0x1:
                        // unset the idx'th bit
                        hash &= ~(0x1 << idx);
                        // set the (idx+1)'th bit
                        hash |= 0x1 << (idx+1);
                        break;
                case 0x2:
                        // set the idx'th bit
                        hash |= 0x1 << idx;
                        break;
                case 0x3:
                        // set the special part of mask for the fourth card
                        hash |= (rank + 1) << 0x1A;
                        break;
                default:
                        PS_UNREACHABLE();
                }
                return hash;
        }

        rank_hash_t rank_hash_create()noexcept{ return 0; }
        rank_hash_t rank_hash_create(rank_vector const& rv)noexcept{
                auto hash = rank_hash_create();
                for(auto id : rv )
                        hash = rank_hash_append(hash, id);
                return hash;
        }
        template<class... Args>
        rank_hash_t rank_hash_create(Args... args)noexcept{
                auto hash = rank_hash_create();
                int _[] = {0,  (hash = rank_hash_append(hash, args),0)...};
                return hash;
        }
        template<class... Args>
        rank_hash_t rank_hash_create_from_cards(Args... args)noexcept{
                auto hash = rank_hash_create();
                int _[] = {0,  (hash = rank_hash_append(hash, card_rank_from_id(args)),0)...};
                return hash;
        }
        const rank_hash_t rank_hash_max(size_t n = 7)noexcept{
                switch(n){
                case 7:
                default:
                        return rank_hash_create(12,12,12,12,11,11,11);
                }
        }
        
        using card_hash_t = std::uint64_t;

        card_hash_t card_hash__detail__pack(suit_hash_t sh, rank_hash_t rh){
                return 
                        (static_cast<card_hash_t>(sh) << 32) |
                         static_cast<card_hash_t>(rh)
                ;
        }
        suit_hash_t card_hash__detail__get_suit(card_hash_t hash){
                return static_cast<suit_hash_t>( hash >> 32 ); 
        }
        rank_hash_t card_hash__detail__get_rank(card_hash_t hash){
                return static_cast<rank_hash_t>( hash & 0xffffffff); 
        }
        card_hash_t card_hash_create(){
                return card_hash__detail__pack(
                        suit_hash_create(),
                        rank_hash_create());
        }
        card_hash_t card_hash_append(card_hash_t hash, card_id id){
                return card_hash__detail__pack(
                        suit_hash_append( card_hash__detail__get_suit(hash), card_suit_from_id(id)),
                        rank_hash_create( card_hash__detail__get_rank(hash), card_rank_from_id(id)));
        }
        template<class... Args>
        card_hash_t card_hash_create_from_cards(Args... cards){
                return card_hash__detail__pack(
                        suit_hash_create_from_cards(cards...),
                        rank_hash_create_from_cards(cards...));
        }




        struct hash_ranker{
                using hash_t = rank_hasher::hash_t;
                hash_ranker(){
                        rank_map_.resize(rhasher_.max());
                        flush_map_.resize(rhasher_.max());
                        std::cout << "systems are go\n";
                }
                void rank_commit(hash_t hash, ranking_t r)noexcept{
                        rank_map_[hash] = r;
                }
                void flush_commit(hash_t hash, ranking_t r)noexcept{
                        flush_map_[hash] = r;
                }
                ranking_t rank_eval(hash_t hash)const noexcept{
                        return rank_map_[hash];
                }
                ranking_t flush_eval(hash_t hash)const noexcept{
                        return flush_map_[hash];
                }
        private:
                suit_hasher shasher_;
                rank_hasher rhasher_;
                std::vector<ranking_t> flush_map_;
                std::vector<ranking_t> rank_map_;
        };

        struct hash_ranker_gen_5{
                void operator()(hash_ranker& hr){
                        struct hash_ranker_maker_detail{
                                hash_ranker_maker_detail(hash_ranker* ptr):ptr_{ptr}{}
                                void begin(std::string const&){}
                                void end(){}
                                void next( bool f, card_id a, card_id b, card_id c, card_id d, card_id e){
                                        auto rhash = rhasher_.create(a,b,c,d,e);
                                        if( f )
                                                ptr_->flush_commit(rhash, order_);
                                        else
                                                ptr_->rank_commit(rhash, order_);
                                        ++order_;
                                }
                                hash_ranker* ptr_;
                                size_t order_{1};
                                suit_hasher shasher_;
                                rank_hasher rhasher_;
                        };
                        hash_ranker_maker_detail aux(&hr);
                        generate(aux);
                        PRINT(aux.order_);
                }
        };





struct evaluator_5_card_hash{
        evaluator_5_card_hash(){
                hash_ranker_gen_5{}(impl_);
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e)const noexcept{
                auto shash = shasher_.create_from_cards(a,b,c,d,e);
                auto rhash = rhasher_.create_from_cards(a,b,c,d,e);
                if( shasher_.has_flush(shash) )
                        return impl_.flush_eval(rhash);
                return impl_.rank_eval(rhash);
        }
        ranking_t rank(card_id a, card_id b, card_id c, card_id d, card_id e, card_id f)const noexcept{
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
        suit_hasher shasher_;
        rank_hasher rhasher_;
        hash_ranker impl_;
};


struct evaluator_7_card_map : evaluator
{
        evaluator_7_card_map(){
                card_map_7_.resize(rhasher_.max());

                for(size_t i=0;i!=52;++i){
                        card_rank_device_[i] = card_decl::get(i).rank().id();
                }

                using iter_t = basic_index_iterator<
                        int, ordered_policy, rank_vector
                >;

                for(iter_t iter(7,13),end;iter!=end;++iter){
                        maybe_add_(*iter);
                }
        }
        ranking_t rank(long a, long b, long c, long d, long e)const override{
                return impl_.rank(a,b,c,d,e);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
                return impl_.rank(a,b,c,d,e,f);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{

                auto shash =  shasher_.create_from_cards(a,b,c,d,e,f,g);

                if( shasher_.has_flush(shash)){
                        //++miss;
                        return impl_.rank(a,b,c,d,e,f,g);
                }

                auto rhash = rhasher_.create_from_cards(a,b,c,d,e,f,g);
                auto ret = card_map_7_[rhash];

                return ret;
        }
        mutable std::atomic_int miss{0};
        mutable std::atomic_int hit{0};
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const {

                if( shasher_.has_flush(suit_hash) ){
                        ++miss;
                        return impl_.rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                ++hit;
                auto ret = card_map_7_[rank_hash];
                return ret;
        }
private:
        ranking_t rank_from_rank_impl_(long a, long b, long c, long d, long e, long f, long g)const{
                return impl_.rank( card_decl::make_id(0,a),
                                    card_decl::make_id(0,b),
                                    card_decl::make_id(0,c),
                                    card_decl::make_id(0,d),
                                    card_decl::make_id(1,e),
                                    card_decl::make_id(1,f),
                                    card_decl::make_id(1,g) );
        }
        ranking_t rank_from_rank_(long a, long b, long c, long d, long e, long f, long g)const{
                return this->rank( card_decl::make_id(0,a),
                                   card_decl::make_id(0,b),
                                   card_decl::make_id(0,c),
                                   card_decl::make_id(0,d),
                                   card_decl::make_id(1,e),
                                   card_decl::make_id(1,f),
                                   card_decl::make_id(1,g) );
        }
        void maybe_add_(rank_vector const& b){
                // first check we don't have more than 4 of each card
                std::array<int, 13> aux = {0};
                for(size_t i=0;i!=7;++i){
                        ++aux[b[i]];
                }
                for(size_t i=0;i!=aux.size();++i){
                        if( aux[i] > 4 )
                                return;
                }
                auto hash = rhasher_.create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                auto val  = rank_from_rank_impl_( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                //std::cout << detail::to_string(aux) << " - " << detail::to_string(b) << " => " << std::bitset<30>(static_cast<unsigned long long>(hash)).to_string() << "\n";
                //

                card_map_7_[hash] = val;
        }
        rank_hasher rhasher_;
        suit_hasher shasher_;
        evaluator_5_card_hash impl_;
        std::array<size_t, 52> card_rank_device_;
        std::vector<ranking_t> card_map_7_;
};



} // working



int main(){
        working::evaluator_5_card_hash ev;
        holdem_class_vector cv;
        working::holdem_board_decl w;
        rank_hasher rh;
        suit_hasher sh;
        #if 0
        for(int i=0;i!=52;++i){
                auto const& card{ card_decl::get(i) };
                std::cout << card 
                        << " - " << (int)card.suit() << " - " << (int)(i & 0x3 )
                        << " / " << (int)card.rank() << " - " << (int)(i >> 2 )
                        << "\n";
        }
        return 0;
        #endif
        #if 1
        cv.push_back("AA");
        cv.push_back("KK");
        #endif
        #if 0
        cv.push_back("AKs");
        cv.push_back("QJs");
        cv.push_back("T9s");
        cv.push_back("87s");
        #endif

        boost::timer::auto_cpu_timer at;
        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(cv.size());
        for( auto hvt : cv.to_standard_form_hands()){
                auto const& perm = std::get<0>(hvt);
                auto const& hv   = std::get<1>(hvt);
                PRINT( hv );
                auto hv_mask = hv.mask();
                #if 1
                        
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
                        auto rank_proto = b.rank_hash();
                        auto suit_proto = b.suit_hash();


                        for(size_t i=0;i!=hv.size();++i){

                                auto rank_hash = rank_proto;
                                auto suit_hash = suit_proto;

                                rank_hash = rh.append(rank_hash, hv_first_rank[i]);
                                rank_hash = rh.append(rank_hash, hv_second_rank[i]);

                                suit_hash = sh.append(suit_hash, hv_first_suit[i] );
                                suit_hash = sh.append(suit_hash, hv_second_suit[i] );

                                using namespace working;

                                #if 0
                                auto card_hash = working::card_hash_create_from_cards(b.board()[0],
                                                                                      b.board()[1],
                                                                                      b.board()[2],
                                                                                      b.board()[3],
                                                                                      b.board()[4],
                                                                                      hv_first[i],
                                                                                      hv_second[i]);
                                                                                      #endif
                                auto suithash2 = suit_hash_create_from_cards(b.board()[0],
                                                                             b.board()[1],
                                                                             b.board()[2],
                                                                             b.board()[3],
                                                                             b.board()[4],
                                                                             hv_first[i],
                                                                             hv_second[i]);
                                auto rankhash2 = rank_hash_create_from_cards(b.board()[0],
                                                                             b.board()[1],
                                                                             b.board()[2],
                                                                             b.board()[3],
                                                                             b.board()[4],
                                                                             hv_first[i],
                                                                             hv_second[i]);
                                auto card_hash = card_hash_create_from_cards(b.board()[0],
                                                                             b.board()[1],
                                                                             b.board()[2],
                                                                             b.board()[3],
                                                                             b.board()[4],
                                                                             hv_first[i],
                                                                             hv_second[i]);


                                PRINT( std::bitset<32>(rank_hash) );
                                PRINT( std::bitset<32>(rankhash2) );
                                PRINT( std::bitset<32>(suit_hash) );
                                PRINT( std::bitset<32>(suithash2) );
                                PRINT( std::bitset<64>(card_hash) );

                                #if 0
                                ranked[i] = ev.rank(b.board(),
                                                    suit_hash, rank_hash,
                                                    hv_first[i],
                                                    hv_second[i]);
                                                    #endif
                                ranked[i] = ev.rank(b.board()[0],
                                                    b.board()[1],
                                                    b.board()[2],
                                                    b.board()[3],
                                                    b.board()[4],
                                                    hv_first[i],
                                                    hv_second[i]);

                        }
                        detail::dispatch_ranked_vector{}(*sub, ranked);

                }
                PRINT(board_count);
                result->append_matrix(*sub, perm );
                #else
                PRINT(detail::to_string(perm));
                PRINT(hv);
                auto ret = ec.evaluate(hv);
                result->append_matrix(*ret, perm);
                #endif
        }
        std::cout << *result << "\n";
        #if 0
        auto r = static_cast<double>(ev.hit)/(ev.miss+ev.hit);
        PRINT_SEQ((ev.hit)(ev.miss)(r));
        #endif
}
