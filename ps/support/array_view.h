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
#ifndef PS_ARRAY_VIEW_H
#define PS_ARRAY_VIEW_H

#include <array>
#include <vector>

namespace ps{
namespace support{

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

                // noexcept should be conditional here
                template<class L, class R>
                friend bool operator==(array_view<L> const& l, array_view<R> const& r)noexcept{
                        return l.size() == r.size() && std::equal(l.begin(), l.end(), r.begin());
                }
                template<class L, class R>
                friend bool operator!=(array_view<L> const& l, array_view<R> const& r)noexcept{
                        return !(l == r );
                }
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
} // supprt
} // ps


namespace boost
{
        template <class T>
        std::size_t hash_value(ps::support::array_view<T> const& v)
        {
                return boost::hash_range(v.begin(), v.end());
        }
} // end namespace boost
#endif // PS_ARRAY_VIEW_H 
