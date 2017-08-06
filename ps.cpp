#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <map>
#include <codecvt>
#include <type_traits>
#include <functional>

#include <boost/range/algorithm.hpp>
#include "ps/detail/visit_combinations.h"

#include "ps/base/range.h"
#include "ps/base/card_vector.h"
#include "ps/eval/evaluator.h"
#include "ps/eval/rank_world.h"
#include "ps/eval/equity_evaulator.h"

using namespace ps;

template<class V, class Vec>
void cross_product_vec( V v, Vec& vec){

        using iter_t = decltype(vec.front().begin());

        size_t const sz = vec.size();

        std::vector<iter_t> proto_vec;
        std::vector<iter_t> iter_vec;
        std::vector<iter_t> end_vec;


        for( auto& e : vec){
                proto_vec.emplace_back( e.begin() );
                end_vec.emplace_back(e.end());
        }
        iter_vec = proto_vec;

        for(;;){
                v(iter_vec);

                size_t cursor = sz - 1;
                for(;;){
                        if( ++iter_vec[cursor] == end_vec[cursor]){
                                if( cursor == 0 )
                                        return;
                                iter_vec[cursor] = proto_vec[cursor];
                                --cursor;
                                continue;
                        }
                        break;
                }
        }
}

#if 0
int main(){
        range hero, villian;
        hero.set_class( holdem_class_decl::parse("KK"));
        hero.set_class( holdem_class_decl::parse("QQ"));
        villian.set_hand( holdem_hand_decl::parse("As2h"));
        villian.set_class( holdem_class_decl::parse("22"));

        std::vector<range> aux{ hero, villian };

        cross_product_vec([](auto const& v){
                PRINT_SEQ((v[0].class_())(v[1].class_()));
                cross_product_vec([](auto const& v){
                              PRINT_SEQ((v[0].decl())(v[1].decl()));
                }, v);
        }, aux);

}
#endif

#if 0
int main(){
        using namespace decl;
        auto const& eval = evaluator_factory::get("6_card_map");
        auto const& rm = rank_word_factory::get();
        PRINT( rm[eval.rank( _Ah, _Kh, _Qh, _Jh, _Th )] );
        PRINT( rm[eval.rank( _Ah, _Kd, _Qh, _Jh, _Th )] );
        PRINT( rm[eval.rank( _Ah, _Kd, _Qh, _Jh, _Th, _2c )] );
        PRINT( rm[eval.rank( _Ah, _Kd, _Qh, _Jh, _Th, _2c, _Ad )] );
        PRINT( rm[eval.rank( _Ah, _Ad, _Qh, _Jh, _Th )] );
}
#endif


struct board_combination_iterator{
        // construct psuedo end iterator
        board_combination_iterator():end_flag_{true}{}

        explicit board_combination_iterator(size_t n, std::vector<card_id> removed = std::vector<card_id>{})
                : removed_{std::move(removed)}
                , n_{n}
        {
                assert( n != 0 && "precondition failed");
                assert( n < 52 && "precondition failed");
                boost::sort(removed_);
                next_.assign(52,-1);
                for(card_id id{52};id!=0;){
                        --id;

                        if( boost::binary_search(removed_, id) ){
                                // removed card, ignore this mapping
                                continue;
                        }

                        if( id == 0 ){
                                // found last first case
                                last_ = id;
                                break;
                        }
                        card_id cand(id-1);
                        // while card is in removed_, decrement
                        for(; cand != 0 && boost::binary_search(removed_, cand);)--cand;
                        if( cand == 0 ){
                                // found last second case
                                last_ = id;
                                break;
                        }
                        next_[id] = cand;
                }
                PRINT(detail::to_string(next_));
                
                for(card_id id{52};id!=0 && board_.size() < n_;){
                        --id;

                        if( boost::binary_search(removed_, id) )
                                continue;
                        board_.emplace_back(id);
                }

                if( board_.size() != n_ )
                        BOOST_THROW_EXCEPTION(std::domain_error("unable to construct board of size n"));

                PRINT(last_);

        }
        auto const& operator*()const{ return board_; }
        board_combination_iterator& operator++(){
                size_t cursor = n_ - 1;
                for(;;){

                        // First see if we can't decrement the board
                        // at the cursor
                        if( cursor == n_ -1 ){
                                if( next_[board_[cursor]] == -1 ){
                                        // case XXXX0
                                        --cursor;
                                        continue;
                                }
                        } else {
                                if( next_[board_[cursor]] == board_[cursor+1] ){
                                        // case XXX10
                                        --cursor;
                                        continue;
                                }
                        }
                        break;
                }
                if( cursor == -1 ){
                        end_flag_ = true;
                        return *this;
                }
                board_[cursor] = next_[board_[cursor]];
                ++cursor;
                for(;cursor != n_;++cursor){
                        board_[cursor] = next_[board_[cursor-1]];
                }
                return *this;
        }

        bool operator==(board_combination_iterator const& that)const{
                return this->end_flag_ && that.end_flag_;
        }
        bool operator!=(board_combination_iterator const& that)const{
                return ! ( *this == that);
        }
private:
        // cards not to be run out on board
        card_vector removed_;
        // mapping m : c -> c, for next, ie when there are non
        // removed we have m = identity map
        std::vector<card_id> next_;
        // in memory rep
        card_vector board_;
        // size of board
        size_t n_;
        // flag to indicate at end
        bool end_flag_ = false;
        // last card, 0 when there are non removed
        card_id last_;
};

#if 0
int main(){
        size_t count=0;
        for(board_combination_iterator iter(3, std::vector<card_id>{0,2,4,5}),end;iter!=end;++iter){
                ++count;
        }
        PRINT(count);
}
#endif

#if 1
struct equity_evaulator_impl : public ::ps::equity_evaluator{
        equity_eval_result evaluate(std::vector<holdem_id> const& players)const override{
                // we first need to enumerate every run of the board,
                // for this we can create a mapping [0,51-n*2] -> [0,51],
                equity_eval_result result{2};

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

                auto const& eval = evaluator_factory::get("5_card_map");
        
                #if 0
                for(board_combination_iterator iter(5, known),end;iter!=end;++iter){

                        auto const& b(*iter);

                        std::vector<ranking_t> ranked;
                        for( int i=0;i!=n;++i){
                                ranked.push_back(eval.rank(x[i], y[i],
                                                            b[0], b[1], b[2], b[3], b[4]) );
                        }
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
                #endif

                #if 1
                boost::sort(known);
                auto filter = [&](long c){ return ! boost::binary_search(known, c); };
                
                detail::visit_combinations<5>( [&](id_type a, id_type b, id_type c, id_type d, id_type e){
                        std::vector<ranking_t> ranked;
                        for( int i=0;i!=n;++i){
                                ranked.push_back(eval.rank(x[i], y[i],
                                                           a,b,c,d,e));
                        }
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
                }, filter, 51);
                #endif

                return std::move(result);
        }
};
#endif

#if 1
int main(){
        using namespace decl;
        // Hand 0: 	60.228%  	59.34% 	00.89% 	       1016051 	    15248.00   { Ts8h }
        // Hand 1: 	39.772%  	38.88% 	00.89% 	        665767 	    15248.00   { 7c6c }
        std::vector<holdem_id> p{ 
                holdem_hand_decl::parse("Ts8h"),
                holdem_hand_decl::parse("7c6c")
        };
        equity_evaulator_impl ee;
        auto ret = ee.evaluate(p);
        PRINT( ret.player(0).nwin(0));
        PRINT( ret.player(0).nwin(1));
        PRINT( ret.player(1).nwin(0));
        PRINT( ret.player(1).nwin(1));
        std::cout << ret << "\n";
        //std::cout << ret << "\n";
}
#endif
