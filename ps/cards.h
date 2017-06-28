#ifndef PS_CORE_CARDS_H
#define PS_CORE_CARDS_H

#include <cassert>
#include <array>
#include <set>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include "ps/detail/void_t.h"
#include "ps/detail/print.h"

#include <boost/exception/all.hpp>
#include <boost/range/algorithm.hpp>

namespace ps{

        #if 0
        using id_type =  std::uint16_t;

        using suit_id   = std::uint8_t;
        using rank_id   = std::uint8_t; 
        using card_id   = std::uint8_t; 
        using holdem_id = std::uint16_t;
        #endif

        using id_type =  unsigned;

        using suit_id   = unsigned;
        using rank_id   = unsigned;
        using card_id   = unsigned;
        using holdem_id = unsigned;
        using holdem_class_id = unsigned;

        // random access, means
        //      {id=0, key=4}, {id=2,key=5} needs
        //      {4,-1,5} etc
        template<class T, class Key = id_type>
        struct decl_factory{
                template<class... Args>
                explicit decl_factory(Args&&... args)
                        : vec_{std::forward<Args>(args)...}
                {
                        boost::sort( vec_ );
                }
                T const& get(Key k)const{
                        assert( k < vec_.size() && "key doesn't exist");
                        return vec_[k];
                }
        private:
                std::vector<T> vec_;
        };


        struct suit_decl{
                suit_decl(suit_id id, char sym, std::string const& name)
                        : id_{id}, sym_{sym}, name_{name}
                {}
                auto id()const{ return id_; }
                std::string to_string()const{ return std::string{sym_}; }
                friend std::ostream& operator<<(std::ostream& ostr, suit_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(suit_decl const& that)const{
                        return id_ < that.id_;
                }
                inline static suit_decl const& get(suit_id id);
                inline static suit_decl const& get(std::string const& s);
                operator suit_id()const{ return id_; }
        private:
                suit_id id_;
                char sym_;
                std::string name_;
        };
        
        struct rank_decl{
                rank_decl(rank_id id, char sym)
                        : id_{id}, sym_{sym}
                {}
                auto id()const{ return id_; }
                std::string to_string()const{ return std::string{sym_}; }
                friend std::ostream& operator<<(std::ostream& ostr, rank_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(rank_decl const& that)const{
                        return id_ < that.id_;
                }
                inline static rank_decl const& get(rank_id id);
                inline static rank_decl const& get(std::string const& s);
                inline static rank_decl const& get(char c){ return get(std::string{c}); }
                operator rank_id()const{ return id_; }
        private:
                rank_id id_;
                char sym_;
        };

        struct card_decl{
                card_decl( suit_decl const& s, rank_decl const& r):
                        id_{make_id(s.id(),r.id())}
                        ,suit_{s}, rank_{r}
                {}
                auto id()const{ return id_; }
                std::string to_string()const{
                        return rank_.to_string() + 
                               suit_.to_string();
                }
                suit_decl const& suit()const{ return suit_; }
                rank_decl const& rank()const{ return rank_; }
                friend std::ostream& operator<<(std::ostream& ostr, card_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(card_decl const& that)const{
                        return id_ < that.id_;
                }
                inline static card_decl const& get(card_id id);
                inline static card_decl const& get(std::string const& s){
                        assert( s.size() == 2 && "precondition failed");
                        return get( rank_decl::get(s.substr(0,1)).id() * 4 +
                                    suit_decl::get(s.substr(1,1)).id()   );
                }
                operator card_id()const{ return id_; }
                static card_id make_id( suit_id s, rank_id r){
                        return s + r * 4;
                }
        private:
                card_id id_;
                suit_decl suit_;
                rank_decl rank_;
        };

        struct holdem_hand_decl{
                // a must be the biggest
                holdem_hand_decl( card_decl const& a, card_decl const& b):
                        id_{ make_id(a.id(), b.id()) },
                        first_{a},
                        second_{b}
                {
                }
                auto id()const{ return id_; }
                std::string to_string()const{
                        return first_.to_string() + 
                               second_.to_string();
                }
                friend std::ostream& operator<<(std::ostream& ostr, holdem_hand_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(holdem_hand_decl const& that)const{
                        return id_ < that.id_;
                }
                decltype(auto) first()const{ return first_; }
                decltype(auto) second()const{ return second_; }
                inline static holdem_hand_decl const& get(holdem_id id);
                inline static holdem_hand_decl const& get(std::string const& s){
                        assert( s.size() == 4 && "precondition failed");
                        auto x = card_decl::get(s.substr(0,2)).id();
                        auto y = card_decl::get(s.substr(2,2)).id();
                        return get(make_id(x,y));
                }
                static holdem_id make_id( rank_id r0, suit_id s0,
                                        rank_id r1, suit_id s1)
                {
                        holdem_id x{ card_decl::make_id(s0, r0) };
                        holdem_id y{ card_decl::make_id(s1, r1) };
                        return make_id(x,y);
                }
                static holdem_id make_id( card_id x, card_id y){
                        return  x * 52 + y;
                }
                operator holdem_id()const{ return id_; }
        private:
                holdem_id id_;
                card_decl first_;
                card_decl second_;

        };

        enum class suit_category{
                any_suit,
                suited,
                offsuit
        };
        
        enum class holdem_class_type{
                pocket_pair,
                suited,
                offsuit
        };

        struct holdem_class_decl{
                holdem_class_decl(holdem_class_type cat,
                                  rank_decl const& a,
                                  rank_decl const& b):
                        id_{ make_id( cat, a.id(), b.id() ) },
                        cat_{cat},
                        first_{a},
                        second_{b}
                {
                        //PRINT_SEQ((id_)(*this));

                        switch(cat_){
                        case holdem_class_type::suited:
                                for(suit_id x{0};x!=4;++x){
                                        hand_set_.emplace_back(
                                                card_decl(
                                                        suit_decl::get(x), first_),
                                                card_decl(
                                                        suit_decl::get(x), second_));
                                }
                                break;
                        case holdem_class_type::offsuit:
                                for(suit_id x{0};x!=4;++x){
                                        for(suit_id y{0};y!=4;++y){
                                                if( x == y )
                                                        continue;
                                                hand_set_.emplace_back(
                                                        card_decl(
                                                                suit_decl::get(x), first_),
                                                        card_decl(
                                                                suit_decl::get(y), second_));
                                        }
                                }
                                break;
                        case holdem_class_type::pocket_pair:
                                for(suit_id x{0};x!=4;++x){
                                        for(suit_id y{x+1};y!=4;++y){
                                                hand_set_.emplace_back(
                                                        card_decl(
                                                                suit_decl::get(x), first_),
                                                        card_decl(
                                                                suit_decl::get(y), first_));
                                        }
                                }
                                break;
                        }
                }
                auto get_hand_set()const{ return hand_set_; }
                auto id()const{ return id_; }
                auto category(){ return cat_; }
                std::string to_string()const{
                        std::string tmp{
                                first_.to_string() + 
                                second_.to_string()};
                        switch(cat_){
                        case holdem_class_type::suited:
                                tmp += 's';
                                break;
                        case holdem_class_type::offsuit:
                                tmp += 'o';
                                break;
                        case holdem_class_type::pocket_pair:
                                break;
                        }
                        return std::move(tmp);
                }
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(holdem_class_decl const& that)const{
                        return id_ < that.id_;
                }
                decltype(auto) first()const{ return first_; }
                decltype(auto) second()const{ return second_; }
                inline static holdem_class_decl const& get(holdem_id id);
                inline static holdem_class_decl const& get(std::string const& s){
                        auto x = rank_decl::get(s.substr(0,1)).id();
                        auto y = rank_decl::get(s.substr(1,1)).id();
                        PRINT_SEQ((s)(x)(y));
                        switch(s.size()){
                        case 2:
                                return get(make_id(holdem_class_type::pocket_pair, x, y) );
                        case 3:
                                switch(s[2]){
                                case 's': 
                                        return get(make_id(holdem_class_type::suited, x, y) );
                                case 'o': 
                                        return get(make_id(holdem_class_type::offsuit, x, y) );
                                default:
                                        BOOST_THROW_EXCEPTION(std::domain_error("not a holdem hand cat (" + s + ")"));
                                }
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("not a holdem hand cat (" + s + ")"));
                        }
                }
                // any bijection will do, nice to keep the mapping within a char
                inline static holdem_class_id make_id(holdem_class_type cat, rank_id x, rank_id y);
                operator holdem_class_id()const{ return id_; }
        private:
                holdem_class_id id_;
                holdem_class_type cat_;
                rank_decl first_;
                rank_decl second_;
                std::vector<holdem_hand_decl> hand_set_;
                std::vector<holdem_id> hand_id_vec_;
        };

        template<class... Args,
                 class = detail::void_t<
                         std::enable_if_t<
                                std::is_same<std::decay_t<Args>, holdem_hand_decl>::value>...
                >
        >
        inline bool disjoint( Args&&... args){
                std::array< holdem_hand_decl const*, sizeof...(args)> aux{ &args...};
                std::set<card_id> s;
                for( auto ptr : aux ){
                        s.insert( ptr->first() );
                        s.insert( ptr->second() );
                }
                return s.size() == aux.size()*2;
        }

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

                #if 0
                static holdem_class_decl _AA{ holdem_class_type::pocket_pair, _A, _A };
                static holdem_class_decl _KK{ holdem_class_type::pocket_pair, _K, _K };
                static holdem_class_decl _QQ{ holdem_class_type::pocket_pair, _Q, _Q };
                static holdem_class_decl _JJ{ holdem_class_type::pocket_pair, _J, _J };
                static holdem_class_decl _TT{ holdem_class_type::pocket_pair, _T, _T };
                static holdem_class_decl _99{ holdem_class_type::pocket_pair, _9, _9 };
                static holdem_class_decl _88{ holdem_class_type::pocket_pair, _8, _8 };
                static holdem_class_decl _77{ holdem_class_type::pocket_pair, _7, _7 };
                static holdem_class_decl _66{ holdem_class_type::pocket_pair, _6, _6 };
                static holdem_class_decl _55{ holdem_class_type::pocket_pair, _5, _5 };
                static holdem_class_decl _44{ holdem_class_type::pocket_pair, _4, _4 };
                static holdem_class_decl _33{ holdem_class_type::pocket_pair, _3, _3 };
                static holdem_class_decl _22{ holdem_class_type::pocket_pair, _2, _2 };
                #endif

        }
                
        suit_decl const& suit_decl::get(suit_id id){
                using namespace decl;
                static decl_factory<suit_decl> fac{_h, _d, _c, _s};
                return fac.get(id);
        }
        suit_decl const& suit_decl::get(std::string const& s){
                assert(s.size()==1 && "preconditon failed");
                using namespace decl;
                switch(s.front()){
                        case 'h': case 'H': return _h;
                        case 'd': case 'D': return _d;
                        case 'c': case 'C': return _c;
                        case 's': case 'S': return _s;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("not a suit (" + s + ")"));
                }
        }
        
        rank_decl const& rank_decl::get(rank_id id){
                using namespace decl;
                static decl_factory<rank_decl> fac{_2,_3,_4,_5,_6,
                                                   _7,_8,_9,_T,_J,
                                                   _Q,_K,_A};
                return fac.get(id);
        }
        rank_decl const& rank_decl::get(std::string const& s){
                assert(s.size()==1 && "preconditon failed");
                using namespace decl;
                switch(s.front()){
                        case '2': return _2;
                        case '3': return _3;
                        case '4': return _4;
                        case '5': return _5;
                        case '6': return _6;
                        case '7': return _7;
                        case '8': return _8;
                        case '9': return _9;
                        case 'T': case 't': return _T;
                        case 'J': case 'j': return _J;
                        case 'Q': case 'q': return _Q;
                        case 'K': case 'k': return _K;
                        case 'A': case 'a': return _A;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("not a rank (" + s + ")"));
                }
        }
        
        card_decl const& card_decl::get(card_id id){
                using namespace decl;
                static decl_factory<card_decl> fac{
                        _Ah, _Kh, _Qh, _Jh, _Th, _9h, _8h, _7h, _6h, _5h, _4h, _3h, _2h,
                        _Ad, _Kd, _Qd, _Jd, _Td, _9d, _8d, _7d, _6d, _5d, _4d, _3d, _2d,
                        _Ac, _Kc, _Qc, _Jc, _Tc, _9c, _8c, _7c, _6c, _5c, _4c, _3c, _2c,
                        _As, _Ks, _Qs, _Js, _Ts, _9s, _8s, _7s, _6s, _5s, _4s, _3s, _2s
                };
                return fac.get(id);
        }
        
        holdem_hand_decl const& holdem_hand_decl::get(holdem_id id){
                static decl_factory<holdem_hand_decl> fac{
                        [](){
                                std::vector< holdem_hand_decl> aux;
                                for( char a{52};a!=0;){
                                        --a;
                                        for( char b{52};b!=0;){
                                                --b;
                                                aux.emplace_back( card_decl::get(a), card_decl::get(b));
                                        }
                                }
                                return std::move(aux);
                        }()
                };
                return fac.get(id);
        }

        holdem_class_id holdem_class_decl::make_id(holdem_class_type cat, rank_id x, rank_id y){
                using std::get;
                static auto aux{[](){
                        std::vector<std::tuple<int,rank_id, rank_id, holdem_class_id> > vec;
                        holdem_class_id id{0};
                        for(rank_id a{13};a!=0;){
                                --a;
                                vec.emplace_back(0,a,a,id++);
                        }
                        for( unsigned a{13};a!=1;){
                                --a;
                                for( unsigned b{a};b!=0;){
                                        --b;
                                        vec.emplace_back(1,a,b,id++);
                                        vec.emplace_back(2,a,b,id++);
                                }
                        }
                        PRINT(vec.size());
                        boost::sort(vec);
                        for( auto const& t : vec ){
                                PRINT_SEQ((get<0>(t))(get<1>(t))(get<2>(t))(get<3>(t)));
                        }
                        return std::move(vec);
                }()};
                if( x < y )
                        std::swap(x,y);
                 
                PRINT_SEQ((static_cast<int>(cat))(x)(y));
                for( auto const& t : aux ){
                        if( get<0>(t) == static_cast<int>(cat) &&
                            get<1>(t) == x &&
                            get<2>(t) == y ){
                                return get<3>(t);
                        }
                }
                BOOST_THROW_EXCEPTION(std::domain_error("not a tuple"));
        }
        holdem_class_decl const& holdem_class_decl::get(holdem_id id){
                static decl_factory<holdem_class_decl> fac{
                        [](){
                                std::vector<holdem_class_decl> aux;
                                for( unsigned a{13};a!=0;){
                                        --a;
                                        aux.emplace_back( holdem_class_type::pocket_pair,
                                                          rank_decl::get(a),
                                                          rank_decl::get(a) );
                                }
                                for( unsigned a{13};a!=1;){
                                        --a;
                                        for( unsigned b{a};b!=0;){
                                                --b;
                                                aux.emplace_back( holdem_class_type::suited,
                                                                  rank_decl::get(a),
                                                                  rank_decl::get(b) );
                                                aux.emplace_back( holdem_class_type::offsuit,
                                                                  rank_decl::get(a),
                                                                  rank_decl::get(b) );
                                        }
                                }
                                return std::move(aux);
                        }()
                };
                return fac.get(id);
        }


                

        #if 0
        namespace literals{

                inline
                holdem_id operator""_h(const char* s, size_t sz){
                        return holdem_hand_decl::get(s).id();
                }
        }
        #endif
                

        // 9 * wins
        // sigma
        // equity
        constexpr size_t computation_size = 11;
        constexpr size_t computation_equity_fixed_prec = 1'000'000;


}

#endif // PS_CORE_CARDS_H
