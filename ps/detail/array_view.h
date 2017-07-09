#ifndef PS_ARRAY_VIEW_H
#define PS_ARRAY_VIEW_H

#include <array>
#include <vector>

namespace ps{
namespace detail{

        template<class T>
        struct array_view{
                using value_type = std::add_const_t<T>;
                array_view(value_type* ptr, size_t sz):
                        ptr_{ptr}, sz_{sz}
                {}
                template<class U, size_t N>
                array_view(std::array<U, N> const& array):
                        ptr_{array.begin()}, sz_{array.size()}
                {
                        static_assert( sizeof(U) == sizeof(value_type), "");
                }
                template<class U>
                array_view(std::vector<U> const& vec):
                        ptr_{&vec[0]}, sz_{vec.size()}
                {
                        static_assert( sizeof(U) == sizeof(value_type), "");
                }
                auto size()const{ return sz_; }
                auto& operator[](size_t idx){ return ptr_[idx]; }
                auto const& operator[](size_t idx)const{ return ptr_[idx]; }
                auto begin()const{ return ptr_; }
                auto end()const{ return ptr_ + sz_; }
        private:
                value_type const* ptr_;
                size_t sz_;
        };
        // homo only
        template<class T, size_t N>
        bool operator<(std::array<T, N> const& l_param,
                       array_view<T> const& r_param){
                if( !( l_param.size() < r_param.size() ) )
                        return false;
                for(size_t i=0;i!=l_param.size();++i){
                        if( !( l_param[i] < r_param[i] ) )
                                return false;
                }
                return true;
        }

        template<class T, size_t N>
        auto make_array_view(std::array<T, N> const& array){
                return array_view<T>(array);
        }
        template<class T>
        auto make_array_view(std::vector<T> const& vec){
                return array_view<T>(vec);
        }
} // detail
} // ps

#endif // PS_ARRAY_VIEW_H 
