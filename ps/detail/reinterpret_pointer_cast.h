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
#ifndef PS_DETAIL_REINTERPRET_POINTER_CAST_H
#define PS_DETAIL_REINTERPRET_POINTER_CAST_H

#include <memory>

namespace ps{
namespace detail{

template< class T, class U > 
std::shared_ptr<T> reinterpret_pointer_cast( const std::shared_ptr<U>& r ) noexcept
{
        auto p = reinterpret_cast<typename std::shared_ptr<T>::element_type*>(r.get());
        return std::shared_ptr<T>(r, p);
}

} // end namespace detail
} // end namespace ps

#endif // PS_DETAIL_REINTERPRET_POINTER_CAST_H
