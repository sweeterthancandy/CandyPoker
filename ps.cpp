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

struct evaluator_7_card_map : evaluator
{
        evaluator_7_card_map(){
                impl_ = &evaluator_factory::get("6_card_map");
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
                return impl_->rank(a,b,c,d,e);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
                return impl_->rank(a,b,c,d,e,f);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{

                auto shash =  shasher_.create_from_cards(a,b,c,d,e,f,g);

                if( shasher_.has_flush(shash)){
                        //++miss;
                        return impl_->rank(a,b,c,d,e,f,g);
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
                auto hash = rhasher_.create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                auto val  = rank_from_rank_impl_( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                //std::cout << detail::to_string(aux) << " - " << detail::to_string(b) << " => " << std::bitset<30>(static_cast<unsigned long long>(hash)).to_string() << "\n";
                //

                card_map_7_[hash] = val;
        }
        rank_hasher rhasher_;
        suit_hasher shasher_;
        evaluator* impl_;
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


/*
        Idea here is that I'm not concerned about
        creating an object representing 5 cards,
        so that we create another object after adding
        2 more cards, ie, 


 */
struct card_chain{};

int main(){
        #if 0
        for(board_combination_iterator iter(5),end;iter!=end;++iter){
                std::cout << *iter << "\n";
        }
        #endif
        working::equity_evaulator_principal ec;
        working::evaluator_7_card_map ev;
        holdem_class_vector cv;
        holdem_board_decl w;
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
        //auto r = static_cast<double>(ev.hit)/(ev.miss+ev.hit);
        //PRINT_SEQ((ev.hit)(ev.miss)(r));
}
