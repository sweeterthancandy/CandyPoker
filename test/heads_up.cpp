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

#if 0
#include "ps/heads_up.h"


int main(){
        using namespace ps;
        using namespace ps::frontend;
        range p0;
        range p1;

        p0 += _AKo;
        p1 += _55;

        equity_cacher ec;
        ec.load("cache.bin");

        /*
                +----+------+----------+-------+
                |Hand|Equity|   Wins   | Ties  |
                +----+------+----------+-------+
                |Ako |45.38%|55,669,464|562,284|
                | 55 |54.62%|67,054,140|562,284|
                +----+------+----------+-------+
        */

        tree_range root{ std::vector<frontend::range>{p0, p1} };
        root.display();
        hu_visitor agg;
        for( auto const& c : root.children ){
                for( auto d : c.children ){
                        auto ret =  ec.visit_boards(d.players) ;
                        std::cout << d << " -> " << ret << "\n";
                        agg.append(ret);
                }
        }

        std::cout << "aggregate -> " << agg << "\n";

}
#endif
