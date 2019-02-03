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
#ifndef PS_FRONTEND_DECL_H
#define PS_FRONTEND_DECL_H

#include "ps/base/frontend.h"

namespace ps{
        namespace frontend{

                #define PS_rank_seq (A)(K)(Q)(J)(T)(9)(8)(7)(6)(5)(4)(3)(2)

                #define PS_make_pocket_pair(r, data, elem) \
                        static auto BOOST_PP_CAT(_, BOOST_PP_CAT(elem, elem)) = pocket_pair(::ps::decl:: BOOST_PP_CAT(_,elem));

                #define PS_detail_cat3(a,b,c) \
                        BOOST_PP_CAT(BOOST_PP_CAT(a,b), c)
                #define PS_detail_cat4(a,b,c,d) \
                        BOOST_PP_CAT(BOOST_PP_CAT(a,b), BOOST_PP_CAT(c,d))
                #define PS_detail_underscore(x) \
                        BOOST_PP_CAT(_, x)

                #define PS_make_non_pair_aux(r, type, suffix, x, y) \
                        static auto PS_detail_underscore( PS_detail_cat3(x,y,suffix) ) = \
                                type( ::ps::decl:: PS_detail_underscore(x),  \
                                      ::ps::decl:: PS_detail_underscore(y) );
                        
                #define PS_make_non_pair(r, product) \
                        PS_make_non_pair_aux(r, offsuit, o, BOOST_PP_SEQ_ELEM(0,product), BOOST_PP_SEQ_ELEM(1,product)) \
                        PS_make_non_pair_aux(r, suited, s, BOOST_PP_SEQ_ELEM(0,product), BOOST_PP_SEQ_ELEM(1,product)) \


                BOOST_PP_SEQ_FOR_EACH( PS_make_pocket_pair,, PS_rank_seq)

                BOOST_PP_SEQ_FOR_EACH_PRODUCT( PS_make_non_pair, (PS_rank_seq)(PS_rank_seq))


                #if 0
                static auto _AA = pocket_pair(::ps::decl::_A);
                static auto _KK = pocket_pair(::ps::decl::_K);
                static auto _QQ = pocket_pair(::ps::decl::_Q);
                static auto _JJ = pocket_pair(::ps::decl::_J);
                static auto _TT = pocket_pair(::ps::decl::_T);
                static auto _99 = pocket_pair(::ps::decl::_9);
                static auto _88 = pocket_pair(::ps::decl::_8);
                static auto _77 = pocket_pair(::ps::decl::_7);
                static auto _66 = pocket_pair(::ps::decl::_6);
                static auto _55 = pocket_pair(::ps::decl::_5);
                static auto _44 = pocket_pair(::ps::decl::_4);
                static auto _33 = pocket_pair(::ps::decl::_3);
                static auto _22 = pocket_pair(::ps::decl::_2);

                static auto _AKo = offsuit(::ps::decl::_A, ::ps::decl::_K);
                static auto _AQo = offsuit(::ps::decl::_A, ::ps::decl::_Q);
                static auto _AJo = offsuit(::ps::decl::_A, ::ps::decl::_J);
                static auto _ATo = offsuit(::ps::decl::_A, ::ps::decl::_T);
                #endif

                #if 0
                PS_make_non_pair_aux( 0, offsuit, o, A, K )
                PS_make_non_pair_aux( 0, offsuit, o, A, Q )
                PS_make_non_pair_aux( 0, offsuit, o, A, J )
                PS_make_non_pair_aux( 0, offsuit, o, A, T )
                #endif

                // specific hands

                static auto _AhKh = hand( ::ps::decl::_A, ::ps::decl::_h,
                                          ::ps::decl::_K, ::ps::decl::_h );

        } // frontend
} // ps

#endif // PS_FRONTEND_DECL_H
