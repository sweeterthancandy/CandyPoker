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
#ifndef PS_DECL_H
#define PS_DECL_H

#include "ps/base/cards.h"

namespace ps{
        namespace decl{
                static suit_decl _h{0, 'h', "heart"  };
                static suit_decl _d{1, 'd', "diamond"};
                static suit_decl _c{2, 'c', "club"   };
                static suit_decl _s{3, 's', "space"  };


                static rank_decl _2{0,  '2'};
                static rank_decl _3{1,  '3'};
                static rank_decl _4{2,  '4'};
                static rank_decl _5{3,  '5'};
                static rank_decl _6{4,  '6'};
                static rank_decl _7{5,  '7'};
                static rank_decl _8{6,  '8'};
                static rank_decl _9{7,  '9'};
                static rank_decl _T{8,  'T'};
                static rank_decl _J{9,  'J'};
                static rank_decl _Q{10, 'Q'};
                static rank_decl _K{11, 'K'};
                static rank_decl _A{12, 'A'};


                static card_decl _Ah{_h, _A};
                static card_decl _Kh{_h, _K};
                static card_decl _Qh{_h, _Q};
                static card_decl _Jh{_h, _J};
                static card_decl _Th{_h, _T};
                static card_decl _9h{_h, _9};
                static card_decl _8h{_h, _8};
                static card_decl _7h{_h, _7};
                static card_decl _6h{_h, _6};
                static card_decl _5h{_h, _5};
                static card_decl _4h{_h, _4};
                static card_decl _3h{_h, _3};
                static card_decl _2h{_h, _2};

                static card_decl _Ad{_d, _A};
                static card_decl _Kd{_d, _K};
                static card_decl _Qd{_d, _Q};
                static card_decl _Jd{_d, _J};
                static card_decl _Td{_d, _T};
                static card_decl _9d{_d, _9};
                static card_decl _8d{_d, _8};
                static card_decl _7d{_d, _7};
                static card_decl _6d{_d, _6};
                static card_decl _5d{_d, _5};
                static card_decl _4d{_d, _4};
                static card_decl _3d{_d, _3};
                static card_decl _2d{_d, _2};

                static card_decl _Ac{_c, _A};
                static card_decl _Kc{_c, _K};
                static card_decl _Qc{_c, _Q};
                static card_decl _Jc{_c, _J};
                static card_decl _Tc{_c, _T};
                static card_decl _9c{_c, _9};
                static card_decl _8c{_c, _8};
                static card_decl _7c{_c, _7};
                static card_decl _6c{_c, _6};
                static card_decl _5c{_c, _5};
                static card_decl _4c{_c, _4};
                static card_decl _3c{_c, _3};
                static card_decl _2c{_c, _2};

                static card_decl _As{_s, _A};
                static card_decl _Ks{_s, _K};
                static card_decl _Qs{_s, _Q};
                static card_decl _Js{_s, _J};
                static card_decl _Ts{_s, _T};
                static card_decl _9s{_s, _9};
                static card_decl _8s{_s, _8};
                static card_decl _7s{_s, _7};
                static card_decl _6s{_s, _6};
                static card_decl _5s{_s, _5};
                static card_decl _4s{_s, _4};
                static card_decl _3s{_s, _3};
                static card_decl _2s{_s, _2};

        } // decl
} // ps

#endif // PS_DECL_H
