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

struct evaluator_5_card_map : evaluator{
        evaluator_5_card_map(){
                flush_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );
                rank_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );

                std::array<int,4> suit_map = { 2,3,5,7 };
                for( size_t i{0};i!=52;++i){
                        flush_device_[i] = suit_map[card_decl::get(i).suit().id()];
                        rank_device_[i] = card_decl::get(i).rank().id();
                }
                generate(*this);
        }
        void begin(std::string const&){}
        void end(){}
        void next( bool f, long a, long b, long c, long d, long e){
                auto m = map_rank(a,b,c,d,e);
                if( f )
                        flush_map_[m] = order_;
                else
                        rank_map_[m] = order_;
                ++order_;
        }
        ranking_t eval_flush(std::uint32_t m)const noexcept{
                return flush_map_[m];
        }
        ranking_t eval_flush(long a, long b, long c, long d, long e)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e]);
                return eval_flush(m);
        }


        ranking_t eval_rank(std::uint32_t m)const noexcept{
                return rank_map_[m];
        }
        ranking_t eval_rank(long a, long b, long c, long d, long e)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e]);
                return eval_rank(m);
        }
        ranking_t eval_rank(long a, long b, long c, long d, long e, long f)const noexcept{
                std::uint32_t m = map_rank( rank_device_[a],rank_device_[b],
                                            rank_device_[c],rank_device_[d],
                                            rank_device_[e],rank_device_[f]);
                return eval_rank(m);
        }

        std::uint32_t map_rank(long a, long b, long c, long d, long e)const{
                //                                 2 3 4 5  6  7  8  9  T  J  Q  K  A
                static std::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                return p[a] * p[b] * p[c] * p[d] * p[e];
        }
        std::uint32_t map_rank(long a, long b, long c, long d, long e, long f)const{
                //                                 2 3 4 5  6  7  8  9  T  J  Q  K  A
                static std::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                return p[a] * p[b] * p[c] * p[d] * p[e] * p[f];
        }

        // public interface
        ranking_t rank(long a, long b, long c, long d, long e)const override{
                auto f_aux =  flush_device_[a] * flush_device_[b] * flush_device_[c] * flush_device_[d] * flush_device_[e] ;
                std::uint32_t m = map_rank( rank_device_[a],
                                            rank_device_[b], 
                                            rank_device_[c],
                                            rank_device_[d], 
                                            rank_device_[e]);
                ranking_t ret;


                switch(f_aux){
                case 2*2*2*2*2:
                case 3*3*3*3*3:
                case 5*5*5*5*5:
                case 7*7*7*7*7:
                        ret = eval_flush(m);
                        break;
                default:
                        ret = eval_rank(m);
                        break;
                }
                //PRINT_SEQ((a)(b)(c)(d)(e)(ret));
                return ret;
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
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
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{
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
protected:
        std::array<int, 52> flush_device_;
        std::array<int, 52> rank_device_;
private:
        size_t order_ = 1;
        std::vector<ranking_t> flush_map_;
        std::vector<ranking_t> rank_map_;
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
        evaluator_5_card_map impl_;
        std::array<size_t, 52> card_rank_device_;
        std::vector<ranking_t> card_map_7_;
};


template<class Impl_Type>
struct equity_evaulator_principal_tpl : public ps::equity_evaluator{
        std::shared_ptr<equity_breakdown> evaluate(std::vector<holdem_id> const& players)const override{
                // we first need to enumerate every run of the board,
                // for this we can create a mapping [0,51-n*2] -> [0,51],
                auto result = std::make_shared<equity_breakdown_matrix>(players.size());

                // vector of first and second card
                std::vector<card_id> x,y;
                std::vector<card_id> known;

                for( auto const& p : players){
                        x.push_back(holdem_hand_decl::get(p).first().id());
                        y.push_back(holdem_hand_decl::get(p).second().id());
                }
                auto n = players.size();

                boost::copy( x, std::back_inserter(known));
                boost::copy( y, std::back_inserter(known));

        
                size_t board_count = 0;
                for(board_combination_iterator iter(5, known),end;iter!=end;++iter){
                        ++board_count;

                        auto const& b(*iter);

                        std::vector<ranking_t> ranked;
                        for( size_t i=0;i!=n;++i){
                                ranked.push_back(impl_->rank(x[i], y[i],
                                                            b[0], b[1], b[2], b[3], b[4]) );
                        }
                        detail::dispatch_ranked_vector{}(*result, ranked);
                }
                PRINT(board_count);

                return result;
        }
protected:
        Impl_Type* impl_;
};

struct equity_evaulator_principal
        : equity_evaulator_principal_tpl<evaluator_7_card_map>
{
        equity_evaulator_principal()
        {
                impl_ = new evaluator_7_card_map;
        }
};
} // working



int main(){
        #if 0
        for(board_combination_iterator iter(5),end;iter!=end;++iter){
                std::cout << *iter << "\n";
        }
        #endif
        working::equity_evaulator_principal ec;
        working::evaluator_7_card_map ev;
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


                                ranked[i] = ev.rank(b.board(),
                                                    suit_hash, rank_hash,
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
        auto r = static_cast<double>(ev.hit)/(ev.miss+ev.hit);
        PRINT_SEQ((ev.hit)(ev.miss)(r));
}
