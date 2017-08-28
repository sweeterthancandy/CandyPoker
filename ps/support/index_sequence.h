#ifndef PS_SUPPORT_INDEX_SEQUENCE_H
#define PS_SUPPORT_INDEX_SEQUENCE_H

#include <cstddef>
#include <vector>

namespace ps{


/*
                00
                01
                10
                11
 */
struct range_policy{
        template<class Vec>
        static void init(Vec& vec, size_t n, size_t m){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = 0;
        }
        template<class Vec>
        static bool next(Vec& vec, size_t m){
                size_t cursor = vec.size() - 1;
                for(;cursor!=-1;){
                        if( vec[cursor] + 1 == m ){
                                --cursor;
                                continue;
                        }

                        break; // <----------------------
                }
                if( cursor == -1 ){
                        // at end
                        return false; 
                }
                ++vec[cursor];
                ++cursor;
                for(;cursor != vec.size();++cursor){
                        vec[cursor] = 0;
                }
                return true;
        }
};

/*
                00
                01
                10
                11
 */
struct ordered_policy{
        template<class Vec>
        static void init(Vec& vec, size_t n, size_t m){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = 0;
        }
        template<class Vec>
        static bool next(Vec& vec, size_t m){
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

struct strict_lower_triangle_policy{
        template<class Vec>
        static void init(Vec& vec, size_t n, size_t m){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = i;
        }
        template<class Vec>
        static bool next(Vec& vec, size_t m){
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
        template<class Vec>
        static void init(Vec& vec, size_t n, size_t m){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = i;
        }
        template<class Vec>
        static bool next(Vec& vec, size_t m){
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
        template<class Vec>
        static void init(Vec& vec, size_t n, size_t m){
                vec.resize(n);
                for(size_t i=0;i!=vec.size();++i)
                        vec[i] = m-1-i;
        }
        template<class Vec>
        static bool next(Vec& vec, size_t m){
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

template<class T, class Policy, class Vec = std::vector<T> >
struct basic_index_iterator{
        using integer_t = T;
        using vector_t  = Vec;
        using policy_t  = Policy;

        // construct psuedo end iterator
        basic_index_iterator():end_flag_{true}{}

        explicit basic_index_iterator(size_t n, size_t m)
                :n_{n}
                ,m_{m}
        {
                policy_t::init(vec_, n_, m_);
        }
        auto const& operator*()const{ return vec_; }
        auto const* operator->()const{ return &vec_; }
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
        bool eos()const{ return end_flag_; }
private:
        size_t n_;
        size_t m_;
        vector_t vec_;
        // flag to indicate at end
        bool end_flag_{false};
};

struct index_iterator_test_driver{
        template<class Policy>
        void run(std::string const& name){
                using iter_t = basic_index_iterator<int, Policy >;
                std::cout << "------ " << name << "------ " << "\n";
                for(iter_t iter(3,4),end;iter!=end;++iter){
                        std::cout << "  " << detail::to_string(*iter) << "\n";
                }
        }
        void run_all(){
                run<range_policy>("range_policy");
                run<ordered_policy>("ordered_policy");
                run<strict_lower_triangle_policy>("strict_lower_triangle_policy");
                run<lower_triangle_policy>("lower_triangle_policy");
                run<strict_upper_triangle_policy>("strict_upper_triangle_policy");
        }
};

/*
------ range_policy------ 
  {0, 0, 0}
  {0, 0, 1}
  {0, 0, 2}
  {0, 0, 3}
  {0, 1, 0}
  {0, 1, 1}
  {0, 1, 2}
  {0, 1, 3}
  {0, 2, 0}
  {0, 2, 1}
  {0, 2, 2}
  {0, 2, 3}
  {0, 3, 0}
  {0, 3, 1}
  {0, 3, 2}
  {0, 3, 3}
  {1, 0, 0}
  {1, 0, 1}
  {1, 0, 2}
  {1, 0, 3}
  {1, 1, 0}
  {1, 1, 1}
  {1, 1, 2}
  {1, 1, 3}
  {1, 2, 0}
  {1, 2, 1}
  {1, 2, 2}
  {1, 2, 3}
  {1, 3, 0}
  {1, 3, 1}
  {1, 3, 2}
  {1, 3, 3}
  {2, 0, 0}
  {2, 0, 1}
  {2, 0, 2}
  {2, 0, 3}
  {2, 1, 0}
  {2, 1, 1}
  {2, 1, 2}
  {2, 1, 3}
  {2, 2, 0}
  {2, 2, 1}
  {2, 2, 2}
  {2, 2, 3}
  {2, 3, 0}
  {2, 3, 1}
  {2, 3, 2}
  {2, 3, 3}
  {3, 0, 0}
  {3, 0, 1}
  {3, 0, 2}
  {3, 0, 3}
  {3, 1, 0}
  {3, 1, 1}
  {3, 1, 2}
  {3, 1, 3}
  {3, 2, 0}
  {3, 2, 1}
  {3, 2, 2}
  {3, 2, 3}
  {3, 3, 0}
  {3, 3, 1}
  {3, 3, 2}
  {3, 3, 3}
------ ordered_policy------ 
  {0, 0, 0}
  {0, 0, 1}
  {0, 0, 2}
  {0, 0, 3}
  {0, 1, 1}
  {0, 1, 2}
  {0, 1, 3}
  {0, 2, 2}
  {0, 2, 3}
  {0, 3, 3}
  {1, 1, 1}
  {1, 1, 2}
  {1, 1, 3}
  {1, 2, 2}
  {1, 2, 3}
  {1, 3, 3}
  {2, 2, 2}
  {2, 2, 3}
  {2, 3, 3}
  {3, 3, 3}
------ strict_lower_triangle_policy------ 
  {0, 1, 2}
  {0, 1, 3}
  {0, 2, 3}
  {1, 2, 3}
------ lower_triangle_policy------ 
  {0, 1, 2}
  {0, 1, 3}
  {0, 2, 2}
  {0, 2, 3}
  {0, 3, 3}
  {1, 1, 1}
  {1, 1, 2}
  {1, 1, 3}
  {1, 2, 2}
  {1, 2, 3}
  {1, 3, 3}
  {2, 2, 2}
  {2, 2, 3}
  {2, 3, 3}
  {3, 3, 3}
------ strict_upper_triangle_policy------ 
  {3, 2, 1}
  {3, 2, 0}
  {3, 1, 0}
  {2, 1, 0}
*/

} // ps

#endif // PS_SUPPORT_INDEX_SEQUENCE_H
