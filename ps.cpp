
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

template<class V>
void generate(V v){
  long _L( 1); // psudo low ace
  long _2( 2);
  long _3( 3);
  long _4( 4);
  long _5( 5);
  long _6( 6);
  long _7( 7);
  long _8( 8);
  long _9( 9);
  long _T(10);
  long _J(11);
  long _Q(12);
  long _K(13);
  long _A(14);

  v.begin("Royal Flush");
  v.next(true, _A, _K, _Q, _J, _T );
  v.end();
  v.begin("Straight Flush");
  for( long a(_K+1); a != _5 ;){
    --a;
    v.next(true, a, a-1, a-2, a-3, a-4);
  }
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
  for( long a(_A+1); a != _5 ;){
    --a;
    for( long b(a); b != _4 ;){
      --b;
      for( long c(b); c != _3 ;){
        --c;
        for( long d(c); d != _2 ;){
          --d;
          for( long e(d); e != _L ;){
            --e;
            if( a - e == 4 )
              continue;
            v.next(true, a, b, c, d, e);
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
        v.next(false,a,a,a,b,c);
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
        v.next(false,a,a,b,b,c);
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
            v.next(false, a, b, c, d, e);
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

#include <iostream>
#include <string>
#include <sstream>

struct printer{
  void next( bool f, long a, long b, long c, long d, long e){
    *sstr_ << ( f ? "f " : "  " )
     << m(a) << m(b) << m(c) << m(d) << m(e) << "\n";
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
  std::string m(long a){
    switch(a){
    case  2: return "2";
    case  3: return "3";
    case  4: return "4";
    case  5: return "5";
    case  6: return "6";
    case  7: return "7";
    case  8: return "8";
    case  9: return "9";
    case 10: return "T";
    case 11: return "J";
    case 12: return "Q";
    case 13: return "K";
    case 1:
    case 14: return "A";
    default: { std::stringstream sstr; sstr << "bad (" << a << ")"; return sstr.str(); }
    }
  }
  std::string name_;
  std::stringstream* sstr_;
  size_t count_;
};


namespace detail{
  struct eval_impl{
    eval_impl():count_(0){}
    void next( bool f, long a, long b, long c, long d, long e){
      std::uint32_t m = map(a,b,c,d,e);
      if( f )
        fm_[m] = count_;
      else
        rm_[m] = count_;
      ++count_;
    }
    void begin(std::string const& name){}
    void end(){}
    std::uint32_t eval(long a, long b, long c, long d, long e){
    }
    std::uint32_t map(long a, long b, long c, long d, long e){
      //                                  2 3 4 5  6  7  8  9  T  J  Q  K  A
      std::array<std::uint32_t> p = {0,37,2,3,5,7,11,13,17,19,13,27,29,31,37};
      return p[a] + p[b] + p[c] + p[d] + p[e];
    }
  private:
    size_t count_;
    std::vector<std::uint32_t> fm_;
    std::vector<std::uint32_t> nm_;
  };

int main(){
  printer p;
  generate(p);
}
