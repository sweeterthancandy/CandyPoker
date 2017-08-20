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

#include <boost/range/algorithm.hpp>


using namespace ps;

struct rank_hasher{
        using hash_t = size_t;

        hash_t create(){ return 0; }
        hash_t create(rank_vector const& rv){
                auto hash = create();
                for(auto id : rv )
                        hash = append(hash, id);
                return hash;
        }
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
        hash_t append(hash_t hash, rank_id rank){
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
};

struct suit_hasher{
        

        using hash_t = size_t;

        hash_t create(){ return 1; }
        hash_t append(hash_t hash, rank_id rank){
                static constexpr const std::array<int,4> suit_map = { 2,3,5,7 };
                return hash * suit_map[rank];
        }
};


namespace working{

struct evaluator_7_card_map : evaluator
{
        evaluator_7_card_map(){
                impl_ = &evaluator_factory::get("6_card_map");
                card_map_7_.resize( make_hash_(12,12,12,12, 11,11,11));
                std::array<int,4> suit_map = { 2,3,5,7 };
                for( size_t i{0};i!=52;++i){
                        flush_device_[i] = suit_map[card_decl::get(i).suit().id()];
                        rank_device_[i] = card_decl::get(i).rank().id();
                }

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
                return impl_->rank(a,b,c,d,e);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
                return impl_->rank(a,b,c,d,e,f);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{

                auto f_aux =  flush_device_[a] * flush_device_[b] *
                              flush_device_[c] * flush_device_[d] *
                              flush_device_[e] * flush_device_[f] *
                              flush_device_[g];

                if( (f_aux % (2*2*2*2*2)) == 0 ||
                    (f_aux % (3*3*3*3*3)) == 0 ||
                    (f_aux % (5*5*5*5*5)) == 0 ||
                    (f_aux % (7*7*7*7*7)) == 0 )
                {
                        //++miss;
                        return impl_->rank(a,b,c,d,e,f,g);
                }

                auto ret = card_map_7_[make_hash_(card_rank_device_[a],
                                                  card_rank_device_[b],
                                                  card_rank_device_[c],
                                                  card_rank_device_[d],
                                                  card_rank_device_[e],
                                                  card_rank_device_[f],
                                                  card_rank_device_[g])];

                return ret;
        }
        mutable std::atomic_int miss{0};
        mutable std::atomic_int hit{0};
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const {
                #if 0
                PRINT(suit_hash);
                PRINT(suit_hash % (2*2*2*2*2));
                PRINT(suit_hash % (3*3*3*3*3));
                PRINT(suit_hash % (5*5*5*5*5));
                PRINT(suit_hash % (7*7*7*7*7));
                #endif

                if( (suit_hash % (2*2*2*2*2)) == 0 ||
                    (suit_hash % (3*3*3*3*3)) == 0 ||
                    (suit_hash % (5*5*5*5*5)) == 0 ||
                    (suit_hash % (7*7*7*7*7)) == 0 )
                {
                        ++miss;
                        return impl_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                ++hit;

                auto ret = card_map_7_[rank_hash];
                return ret;
        }
private:
        ranking_t rank_from_rank_impl_(long a, long b, long c, long d, long e, long f, long g)const{
                return impl_->rank( card_decl::make_id(0,a),
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
                auto hash = make_hash_( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                if( debugger_.count(hash) !=0  ){
                        //std::cout << "not injective\n";
                        return;
                }
                debugger_.emplace(hash, b);

                auto val  = rank_from_rank_impl_( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                //std::cout << detail::to_string(aux) << " - " << detail::to_string(b) << " => " << std::bitset<30>(static_cast<unsigned long long>(hash)).to_string() << "\n";
                //

                card_map_7_[hash] = val;
        }
        size_t make_hash_(long a, long b, long c, long d, long e, long f, long g)const{
                static rank_hasher rh;
                auto hash = rh.create();
                hash = rh.append(hash, a);
                hash = rh.append(hash, b);
                hash = rh.append(hash, c);
                hash = rh.append(hash, d);
                hash = rh.append(hash, e);
                hash = rh.append(hash, f);
                hash = rh.append(hash, g);
                return hash;
        } 
        std::map<size_t, rank_vector> debugger_;
        evaluator* impl_;
        std::array<size_t, 52> card_rank_device_;
        std::vector<ranking_t> card_map_7_;
                
        std::array<int, 52> flush_device_;
        std::array<int, 52> rank_device_;
};

namespace detail{
        struct dispatch_ranked_vector{
                void operator()(equity_breakdown_matrix& result, 
                                std::vector<ranking_t> const& ranked)const
                {
                        auto lowest = ranked[0] ;
                        size_t count{1};
                        for(size_t i=1;i<ranked.size();++i){
                                if( ranked[i] == lowest ){
                                        ++count;
                                } else if( ranked[i] < lowest ){
                                        lowest = ranked[i]; 
                                        count = 1;
                                }
                        }
                        for(size_t i=0;i!=ranked.size();++i){
                                if( ranked[i] == lowest ){
                                        ++result.data_access(i,count-1);
                                }
                        }
                        ++result.sigma();
                }
        };
} // detail

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


struct board_world{
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

        board_world(){
                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        world_.emplace_back( *iter );
                }
                PRINT(world_.size());
        }
        auto begin()const{ return world_.begin(); }
        auto end()const{ return world_.end(); }

private:
        std::vector<layout> world_;
};

auto from_bitmask(size_t mask){
        card_vector vec;
        for(size_t i=0;i!=52;++i){
                if( mask & card_decl::get(i).mask() ){
                        vec.push_back(i);
                }
        }
        return std::move(vec);
}
int main(){
        working::equity_evaulator_principal ec;
        working::evaluator_7_card_map ev;
        holdem_class_vector cv;
        board_world w;
        rank_hasher rh;
        suit_hasher sh;
        #if 0
        cv.push_back("AA");
        cv.push_back("KK");
        #endif
        cv.push_back("AKs");
        cv.push_back("QJs");
        cv.push_back("T9s");
        cv.push_back("87s");

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
                                                    #if 0
                                                    b.board()[0],
                                                    b.board()[1],
                                                    b.board()[2],
                                                    b.board()[3],
                                                    b.board()[4],
                                                    #endif
                                                    hv_first[i],
                                                    hv_second[i]);
                        }
                        working::detail::dispatch_ranked_vector{}(*sub, ranked);

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
