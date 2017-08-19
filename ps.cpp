#include <iostream>
#include <thread>
#include <atomic>
#include <numeric>
#include <future>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/detail/print.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/sim/holdem_class_strategy.h"


/*
                Strategy in the form 
                        vpip/fold
 */
#include <boost/range/algorithm.hpp>

/*
        Need to,

                iterate over boards
                        abcde  where a < b < c < d < e ( think strict lower triangle )
                iterator over hands
                        ab  where a <= b               ( think lower triable )


 */
struct strict_lower_triangle_policy{
        template<class T>
        static void init(std::vector<T>& vec, size_t n){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = i;
        }
        template<class T>
        static bool next(std::vector<T>& vec, size_t m){
                size_t cursor = vec.size() - 1;
                for(;cursor!=-1;){

                        // First see if we can't decrement the board
                        // at the cursor
                        if( cursor == vec.size() -1 ){
                                if( vec[cursor] + 1 == m ){
                                        // case XXXX0
                                        --cursor;
                                        continue;
                                }
                        } else {
                                if( vec[cursor] + 1 == vec[cursor+1] ){
                                        // case XXX10
                                        --cursor;
                                        continue;
                                }
                        }
                        break;
                }
                if( cursor == -1 ){
                        // at end
                        return false; 
                }
                ++vec[cursor];
                ++cursor;
                for(;cursor != vec.size();++cursor){
                        vec[cursor] = vec[cursor-1] + 1;
                }
                return true;
        }
};

struct lower_triangle_policy{
        template<class T>
        static void init(std::vector<T>& vec, size_t n){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = i;
        }
        template<class T>
        static bool next(std::vector<T>& vec, size_t m){
                size_t cursor = vec.size() - 1;
                for(;cursor!=-1;){

                        // First see if we can't decrement the board
                        // at the cursor
                        if( cursor == vec.size() -1 ){
                                if( vec[cursor] + 1 == m ){
                                        // case XXXX0
                                        --cursor;
                                        continue;
                                }
                        } else {
                                if( vec[cursor] == vec[cursor+1] )
                                {
                                        // case XXX10
                                        --cursor;
                                        continue;
                                }
                        }
                        break;
                }
                if( cursor == -1 ){
                        // at end
                        return false; 
                }
                ++vec[cursor];
                ++cursor;
                for(;cursor != vec.size();++cursor){
                        vec[cursor] = vec[cursor-1];
                }
                return true;
        }
};

struct strict_upper_triangle_policy{
        template<class T>
        static void init(std::vector<T>& vec, size_t n, size_t m){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = m-1-i;
        }
        template<class T>
        static bool next(std::vector<T>& vec, size_t m){
                size_t cursor = vec.size() - 1;
                for(;cursor!=-1;){

                        // First see if we can't decrement the board
                        // at the cursor
                        if( cursor == vec.size() -1 ){
                                if( vec[cursor] == 0 ){
                                        // case XXXX0
                                        --cursor;
                                        continue;
                                }
                        } else {
                                if( vec[cursor] - 1 == vec[cursor+1] ){
                                        // case XXX10
                                        --cursor;
                                        continue;
                                }
                        }
                        break;
                }
                if( cursor == -1 ){
                        // at end
                        return false; 
                }
                --vec[cursor];
                ++cursor;
                for(;cursor != vec.size();++cursor){
                        vec[cursor] = vec[cursor-1] - 1;
                }
                return true;
        }
};

template<class Policy>
struct basic_index_iterator{
        using integer_t = int;
        using policy_t = Policy;
        // construct psuedo end iterator
        basic_index_iterator():end_flag_{true}{}

        explicit basic_index_iterator(size_t n, size_t m)
                :n_{n}
                ,m_{m}
        {
                policy_t::init(vec_, n_, m_);
        }
        auto const& operator*()const{ return vec_; }
        basic_index_iterator& operator++(){
                end_flag_ = (  ! policy_t::next(vec_, m_) );
                return *this;
        }

        bool operator==(basic_index_iterator const& that)const{
                return this->end_flag_ && that.end_flag_;
        }
        bool operator!=(basic_index_iterator const& that)const{
                return ! ( *this == that);
        }
private:
        size_t n_;
        size_t m_;
        std::vector<integer_t> vec_;
        // flag to indicate at end
        bool end_flag_{false};
};

using index_iterator = basic_index_iterator<strict_upper_triangle_policy >;

int main(){
        for( index_iterator iter(3,4),end;iter!=end;++iter){
                PRINT( ps::detail::to_string(*iter) );
        }
}
