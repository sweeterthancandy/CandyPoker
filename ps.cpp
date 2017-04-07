
// royal flush
// straight flush
// quad
// full house
// flush
// straight
// trips
// double pair
// pair
// high card

#pragma warning( disable:4530 )

#include <boost/preprocessor.hpp>

#define PRINT_SEQ_detail(r, d, i, e) do{ std::cout << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define PRINT_SEQ(SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( PRINT_SEQ_detail, ~, SEQ) std::cout << "\n"; }while(0)
#define PRINT(X) PRINT_SEQ((X))
#define PRINT_TEST( EXPR ) do{ bool ret(EXPR); std::cout << std::setw(50) << std::left << #EXPR << std::setw(0) << ( ret ? " OK" : " FAILED" ) << "\n"; }while(0);

#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <boost/array.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/tuple/tuple.hpp>



template<class Traits, class V>
void generate(V& v){
        static long _L( 0); // psudo low ace
        static long _2( 1);
        static long _3( 2);
        static long _4( 3);
        static long _5( 4);
        static long _6( 5);
        static long _7( 6);
        static long _8( 7);
        static long _9( 8);
        static long _T( 9);
        static long _J(10);
        static long _Q(11);
        static long _K(12);
        static long _A(13);

        Traits traits;

        boost::array<long, 14> m = { 
                traits.map_rank('A'),
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
        for( long a(_K+1); a != _5 ;){
                --a;
                v.next(true, m[a], m[a-1], m[a-2], m[a-3], m[a-4]);
        }
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
                                                v.next(true, m[a], m[b], m[c], m[d], m[e]);
                                        }
                                }
                        }
                }
        }
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
                        for( long c(_A+1); c != _3 ;){
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
                                                v.next(false, m[a], m[b], m[c], m[d], m[e]);
                                        }
                                }
                        }
                }
        }
        v.end();
}

#if 0
struct rank_map{
  void operator()(
};

struct eval{
  using val_type = std::uint32_t;
  using card_type = std::uint8_t;

  void operator()(std::array<card_type, 5> const& vec)const{
  auto h{ m_(vec) };
    if( is_flush_( vec ) ){
      return flush_vec_[h];
    }
    return rank_vec_[h];
  }
private:
  detail::flush_map fm_;
  detail::rank_map rm_;
  std::vector<val_type> rank_vec_;
  std::vector<val_type> flush_vec_;
};

#endif



struct card_traits{
        long make(std::string const& h)const{
                assert(h.size()==2 && "preconditon failed");
                return make(h[0], h[1]);
        }
        long make(char r, char s)const{
                return map_suit(s)*13 + map_rank(r);
        }
        long suit(long c)const{
                return c / 13;
        }
        long rank(long c)const{
                return c % 13;
        }
        long map_suit(char s)const{
                switch(s){
                case 'h': case 'H': return 0;
                case 'd': case 'D': return 1;
                case 'c': case 'C': return 2;
                case 's': case 'S': return 3;
                default:
                                    assert( 0 && "precondtion failed");
                                    return -1;
                }
        }
        long map_rank(char s)const{
                switch(s){
                case '2': return 0;
                case '3': return 1;
                case '4': return 2;
                case '5': return 3;
                case '6': return 4;
                case '7': return 5;
                case '8': return 6;
                case '9': return 7;
                case 'T': case 't': return 8;
                case 'J': case 'j': return 9;
                case 'Q': case 'q': return 10;
                case 'K': case 'k': return 11;
                case 'A': case 'a': return 12;
                default: return -1;
                }
        }
        std::string suit_to_string(long s)const{
                switch(suit(s)){
                case 0: return "H";
                case 1: return "D";
                case 2: return "C";
                case 3: return "S";
                default:
                        assert(0 && "precondition failed");
                        return "_";
                }
        }
        std::string rank_to_string(long r)const{
                switch(rank(r)){
                case  0: return "2";
                case  1: return "3";
                case  2: return "4";
                case  3: return "5";
                case  4: return "6";
                case  5: return "7";
                case  6: return "8";
                case  7: return "9";
                case  8: return "T";
                case  9: return "J";
                case 10: return "Q";
                case 11: return "K";
                case 12: return "A";
                default:
                        assert(0 && "precondition failed");
                        return "_";
                }
        }
        std::string to_string(long c){
                return rank_to_string(rank(c)) + suit_to_string(suit(c));
        }
};


struct printer{
        printer():order_{1}{}

        void next( bool f, long a, long b, long c, long d, long e){
                *sstr_ << ( f ? "f " : "  " )
                        << traits_.rank_to_string(a)
                        << traits_.rank_to_string(b) 
                        << traits_.rank_to_string(c) 
                        << traits_.rank_to_string(d) 
                        << traits_.rank_to_string(e)
                        << "  => " << order_
                        << "\n";
                ++order_;
                ++count_;
        }
        void begin(std::string const& name){
                name_ = name;
                sstr_ = new std::stringstream;
                count_ = 0;
        }
        void end(){
                std::cout << "BEGIN " << name_ << " - " << count_ << "\n";
                std::cout << sstr_->str();
                std::cout << "END\n\n";
        }
private:
        size_t order_;
        std::string name_;
        std::stringstream* sstr_;
        size_t count_;
        card_traits traits_;
};

struct deck{
        deck(){
                for(long i=0;i!=52;++i)
                        cards_.push_back(i);
        }
        long deal(){
                long c = cards_.back();
                return c;
        }
private:
        std::vector<long> cards_;
};


namespace detail{
        struct eval_impl{
                eval_impl():order_{1}{
                        m_.resize( 37 * 37 * 37 * 37 * 31 * 2 +1 );
                }
                void next( bool f, long a, long b, long c, long d, long e){
                        std::uint32_t m = map(f,a,b,c,d,e);
                        std::cout << ( f ? "f " : "  " )
                                << traits_.rank_to_string(a)
                                << traits_.rank_to_string(b) 
                                << traits_.rank_to_string(c) 
                                << traits_.rank_to_string(d) 
                                << traits_.rank_to_string(e)
                                << "  ~  " << m
                                << "  => " << order_
                                << "\n";
                        m_[m] = order_;
                        ++order_;
                }
                void begin(std::string const& name){
                        name_ = name;
                }
                void end(){}
                std::uint32_t eval(long a, long b, long c, long d, long e)const{
                        bool f{traits_.suit(a) == traits_.suit(b) &&
                            traits_.suit(a) == traits_.suit(c) &&
                            traits_.suit(a) == traits_.suit(d) &&
                            traits_.suit(a) == traits_.suit(e)};
                        std::uint32_t m = map(f,a,b,c,d,e);
                        PRINT(m);
                        return m_[m];
                }
                std::uint32_t map(bool f, long a, long b, long c, long d, long e)const{
                        //                                   2 3 4 5  6  7  8  9  T  J  Q  K  A
                        boost::array<std::uint32_t, 13> p = {2,3,5,7,11,13,17,19,23,27,29,31,37};
                        return ( f ? 2 : 1 ) * 
                                p[traits_.rank(a)] * p[traits_.rank(b)] * 
                                p[traits_.rank(c)] * p[traits_.rank(d)] * p[traits_.rank(e)];
                }
        private:
                size_t order_;
                std::string name_;
                std::vector<std::uint32_t> m_;
                card_traits traits_;
        };
}
struct eval{
        eval(){
                generate<card_traits>(impl_);
        }
        std::uint32_t eval_5(std::vector<long> const& cards)const{
                assert( cards.size() == 5 && "precondition failed");
                return this->operator()(cards[0], cards[1], cards[2], cards[3], cards[4] );
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e)const{
                std::cout
                        << traits_.rank_to_string(a)
                        << traits_.rank_to_string(b) 
                        << traits_.rank_to_string(c) 
                        << traits_.rank_to_string(d) 
                        << traits_.rank_to_string(e) << "\n";
                return impl_.eval(a,b,c,d,e);
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e, long f)const{
                std::vector<std::uint32_t> aux;
                aux.push_back( (*this)(  b,c,d,e,f));
                aux.push_back( (*this)(a,  c,d,e,f));
                aux.push_back( (*this)(a,b,  d,e,f));
                aux.push_back( (*this)(a,b,c,  e,f));
                aux.push_back( (*this)(a,b,c,d,  f));
                aux.push_back( (*this)(a,b,c,d,e  ));
                boost::sort(aux);
                return aux.front();
        }
        std::uint32_t operator()(long a, long b, long c, long d, long e, long f, long g)const{
                std::vector<std::uint32_t> aux;
                aux.push_back( (*this)(  b,c,d,e,f,g));
                aux.push_back( (*this)(a,  c,d,e,f,g));
                aux.push_back( (*this)(a,b,  d,e,f,g));
                aux.push_back( (*this)(a,b,c,  e,f,g));
                aux.push_back( (*this)(a,b,c,d,  f,g));
                aux.push_back( (*this)(a,b,c,d,e,  g));
                aux.push_back( (*this)(a,b,c,d,e,f  ));
                boost::sort(aux);
                return aux.front();
        }
private:
        detail::eval_impl impl_;
        card_traits traits_;
};

struct driver{
        std::uint32_t eval_5(std::string const& str)const{
                assert( str.size() == 10 && "precondition failed");
                std::vector<long> hand;
                for(size_t i=0;i!=10;i+=2){
                        hand.push_back(traits_.make(str[i], str[i+1]) );
                }
                return eval_.eval_5(hand);
        }
private:
        eval eval_;
        card_traits traits_;
};

struct an_result_t{
        size_t wins;
        size_t lose;
        size_t draw;
};

void traits_test(){
        const char* cards [] = {
                "As", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "Ts", "js", "qs", "Ks",
                "AD", "2D", "3D", "4D", "5D", "6D", "7D", "8D", "9D", "TD", "jD", "qD", "KD",
                "Ac", "2c", "3c", "4c", "5c", "6c", "7c", "8c", "9c", "Tc", "jc", "qc", "Kc",
                "Ah", "2h", "3h", "4h", "5h", "6h", "7h", "8h", "9h", "Th", "jh", "qh", "Kh" };

        card_traits t;
        for(long i=0;i!=sizeof(cards)/sizeof(void*);++i){
                long c = t.make(cards[i]);
                std::cout << cards[i] << " -> " << c << " " << t.rank(c) << " - " << t.suit(c) << "\n";
        }
        PRINT( t.make("Ac") );
        PRINT( t.make("5s") );
        PRINT( t.rank(t.make("Ah")) );
        PRINT( t.rank(t.make("Ac")) );
        PRINT( t.suit(t.make("Ah")) );
        PRINT( t.suit(t.make("Ac")) );
}

void eval_test(){
        driver d;

        PRINT( d.eval_5("AhKhQhJhTh") );
        PRINT( d.eval_5("AsKsQsJsTs") );
        PRINT( d.eval_5("AcKcQcJcTc") );
        PRINT( d.eval_5("AhAcAcAd2c") );
        PRINT( d.eval_5("AhAcAcAd2d") );
}


void generate_test(){
        printer p;
        generate<card_traits>(p);

}

int main(){
        generate_test();
        traits_test();
        eval_test();
}
