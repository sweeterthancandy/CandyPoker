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
#ifndef PS_GENERATE_H
#define PS_GENERATE_H

namespace ps{

/*
 * This has no dependancies on other parts of the code, so that it can be
 * used in other projects
 *
 *    v.begin(rank_name)
 *    v.next(is_flush, r0, r1, r2, r3, r4)
 */
template<class V>
void visit_poker_rankings(V& v){
        static long _2( 0);
        static long _3( 1);
        static long _4( 2);
        static long _5( 3);
        static long _6( 4);
        static long _7( 5);
        static long _8( 6);
        static long _9( 7);
        static long _T( 8);
        static long _J( 9);
        static long _Q(10);
        static long _K(11);
        static long _A(12);

        v.begin("Royal Flush");
        v.next(true, _A, _K, _Q, _J, _T );
        v.end();
        v.begin("Straight Flush");
        for( long a(_K+1); a != _6 ;){
                --a;
                v.next(true, a, a-1, a-2, a-3, a-4);
        }
        v.next(true, _5, _4, _3, _2, _A );
        v.end();
        v.begin("Quads");
        for( long a(_A+1); a != _2 ;){
                --a;
                for( long b(_A+1); b != _2 ;){
                        --b;
                        if( a == b )
                                continue;
                        v.next(false, a, a, a, a, b);
                }
        }
        v.end();
        // full house
        v.begin("Full House");
        for( long a(_A+1); a != _2 ;){
                --a;
                for( long b(_A+1); b != _2 ;){
                        --b;
                        if( a == b )
                                continue;
                        v.next(false, a, a, a, b, b);
                }
        }
        v.end();
        v.begin("Flush");
        for( long a(_A+1); a != _6 ;){
                --a;
                for( long b(a); b != _5 ;){
                        --b;
                        for( long c(b); c != _4 ;){
                                --c;
                                for( long d(c); d != _3 ;){
                                        --d;
                                        for( long e(d); e != _2 ;){
                                                --e;
                                                if( a - e == 4 )
                                                        continue;
                                                // don't remap the stright flush
                                                if( a == _A && b == _5 &&  ( b - e ) == 3 )
                                                        continue;
                                                v.next(true, a, b, c, d, e);
                                        }
                                }
                        }
                }
        }
        v.end();
        v.begin("Straight");
        for( long a(_A+1); a != _6 ;){
                --a;
                v.next(false, a, a-1, a-2, a-3, a-4);
        }
        v.next(false, _5, _4, _3, _2, _A );
        v.end();
        v.begin("Trips");
        for( long a(_A+1); a != _2 ;){
                --a;
                for( long b(_A+1); b != _3 ;){
                        --b;
                        if( a == b )
                                continue;
                        for( long c(b); c != _2 ;){
                                --c;
                                if( a == c)
                                        continue;
                                v.next(false, a, a, a, b, c);
                        }
                }
        }
        v.end();
        v.begin("Two pair");
        for( long a(_A+1); a != _3 ;){
                --a;
                for( long b(a); b != _2 ;){
                        --b;
                        for( long c(_A+1); c != _2 ;){
                                --c;
                                if( c == a || c == b )
                                        continue;
                                v.next(false, a, a, b, b, c);
                        }
                }
        }
        v.end();
        v.begin("One pair");
        for( long a(_A+1); a != _2 ;){
                --a;
                for( long b(_A+1); b != _4 ;){
                        --b;
                        if( a == b )
                                continue;
                        for( long c(b); c != _3 ;){
                                --c;
                                if( a == c )
                                        continue;
                                for( long d(c); d != _2 ;){
                                        --d;
                                        if( a == d )
                                                continue;
                                        v.next(false, a, a, b, c, d);
                                }
                        }
                }
        }
        v.end();
        v.begin("High Card");
        for( long a(_A+1); a != _7 ;){
                --a;
                for( long b(a); b != _5 ;){
                        --b;
                        for( long c(b); c != _4 ;){
                                --c;
                                for( long d(c); d != _3 ;){
                                        --d;
                                        for( long e(d); e != _2 ;){
                                                --e;
                                                if( a - e == 4 )
                                                        continue;
                                                if( a == _A && b == _5 &&  ( b - e ) == 3 )
                                                        continue;
                                                v.next(false, a, b, c, d, e);
                                        }
                                }
                        }
                }
        }
        v.end();
}

} // namespace ps

#endif // PS_GENERATE_H
