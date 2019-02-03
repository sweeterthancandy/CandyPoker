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
#ifndef PS_DETAIL_VISIT_SEQUENCE_H
#define PS_DETAIL_VISIT_SEQUENCE_H

#include <utility>

namespace ps{
namespace detail{

using iseq_2_9 = std::integer_sequence<size_t,2,3,4,5,6,7,8,9>;

template<class F, class T, T... values >
void visit_iseq_detail( std::integer_sequence<T, values...>, F f){
        int dummy[] = { 0, (f(boost::mpl::size_t<values>{}, values...),0)...};
}
template<class Seq, class F>
void visit_iseq( F f ){
        visit_iseq_detail(Seq{}, f);
}


template<class F>
void visit_iseq_2_9(F&& f){
        visit_iseq<iseq_2_9>(f);
}



} // defail
} // ps

#if 0
        visit_iseq_2_9( [](auto n){
                constexpr const size_t N = decltype(n)::value;
                PRINT(n);
        });


#endif



#endif // PS_DETAIL_VISIT_SEQUENCE_H
