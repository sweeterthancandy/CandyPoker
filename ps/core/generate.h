#ifndef PS_GENERATE_H
#define PS_GENERATE_H

namespace ps{

template<class Traits, class V>
void generate(V& v){
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

        Traits traits;

        boost::array<long, 13> m = { 
                traits.map_rank('2'),
                traits.map_rank('3'),
                traits.map_rank('4'),
                traits.map_rank('5'),
                traits.map_rank('6'),
                traits.map_rank('7'),
                traits.map_rank('8'),
                traits.map_rank('9'),
                traits.map_rank('T'),
                traits.map_rank('J'),
                traits.map_rank('Q'),
                traits.map_rank('K'),
                traits.map_rank('A')
        };

        v.begin("Royal Flush");
        v.next(true, m[_A], m[_K], m[_Q], m[_J], m[_T] );
        v.end();
        v.begin("Straight Flush");
        for( long a(_K+1); a != _6 ;){
                --a;
                v.next(true, m[a], m[a-1], m[a-2], m[a-3], m[a-4]);
        }
        v.next(true, m[_5], m[_4], m[_3], m[_2], m[_A] );
        v.end();
        v.begin("Quads");
        for( long a(_A+1); a != _2 ;){
                --a;
                for( long b(_A+1); b != _2 ;){
                        --b;
                        if( a == b )
                                continue;
                        v.next(false, m[a], m[a], m[a], m[a], m[b]);
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
                        v.next(false, m[a], m[a], m[a], m[b], m[b]);
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
                                                v.next(true, m[a], m[b], m[c], m[d], m[e]);
                                        }
                                }
                        }
                }
        }
        v.end();
        v.begin("Straight");
        for( long a(_A+1); a != _6 ;){
                --a;
                v.next(false, m[a], m[a-1], m[a-2], m[a-3], m[a-4]);
        }
        v.next(false, m[_5], m[_4], m[_3], m[_2], m[_A] );
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
                                v.next(false, m[a], m[a], m[a], m[b], m[c]);
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
                                v.next(false, m[a], m[a], m[b], m[b], m[c]);
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
                                        v.next(false, m[a], m[a], m[b], m[c], m[d]);
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
                                                v.next(false, m[a], m[b], m[c], m[d], m[e]);
                                        }
                                }
                        }
                }
        }
        v.end();
}

} // namespace ps

#endif // PS_GENERATE_H
