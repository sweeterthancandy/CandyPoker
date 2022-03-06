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
#ifndef PS_DETAIL_DISPATCH_H
#define PS_DETAIL_DISPATCH_H

namespace ps{
namespace detail{
        template<class MatrixType, class ArrayType>
        void dispatch_ranked_vector_mat(MatrixType& result, ArrayType const& ranked, size_t n, size_t weight = 1)noexcept{
                auto lowest = ranked[0] ;
                size_t count{1};
                auto ptr = &result(0, 0);
                for(size_t i=1;i<n;++i){
                        if( ranked[i] == lowest ){
                                ++count;
                                ptr = nullptr;
                        } else if( ranked[i] < lowest ){
                                lowest = ranked[i]; 
                                count = 1;
                                ptr = &result(0, i);
                        }
                }
                if (ptr)
                {
                        *ptr += weight;
                }
                else
                {
                        for(size_t i=0;i!=n;++i){
                                if( ranked[i] == lowest ){
                                        result(count-1, i) += weight;
                                }
                        }
                }
               
        }
} // detail
} // ps

#endif // PS_DETAIL_DISPATCH_H
