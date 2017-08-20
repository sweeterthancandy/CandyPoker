#ifndef PS_EVAL_EVALUATOR_7_CARD_MAP_H
#define PS_EVAL_EVALUATOR_7_CARD_MAP_H

#include <bitset>

#include "ps/detail/print.h"
#include "ps/eval/evaluator.h"
#include "ps/support/config.h"

namespace ps{

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
        size_t make_hash_(long a, long b, long c, long d, long e, long f, long g)const{
                size_t hash = 0;

                hash_add_(hash, a);
                hash_add_(hash, b);
                hash_add_(hash, c);
                hash_add_(hash, d);
                hash_add_(hash, e);
                hash_add_(hash, f);
                hash_add_(hash, g);

                return hash;
        } 
        void hash_add_(size_t& hash, size_t rank)const{
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
        }
        std::map<size_t, rank_vector> debugger_;
        evaluator* impl_;
        std::array<size_t, 52> card_rank_device_;
        std::vector<ranking_t> card_map_7_;
                
        std::array<int, 52> flush_device_;
        std::array<int, 52> rank_device_;
};

} // ps

#endif // PS_EVAL_EVALUATOR_7_CARD_MAP_H
