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
#ifndef PS_DETAIL_CROSS_PRODUCT_H
#define PS_DETAIL_CROSS_PRODUCT_H

#include <vector>
#include <cstddef>

namespace ps{
namespace detail{

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

} // detail
} // ps

#endif // PS_DETAIL_CROSS_PRODUCT_H
