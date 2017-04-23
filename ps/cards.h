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

#include <boost/exception/all.hpp>

namespace ps{

        using id_type = unsigned;

        using suit_id = id_type;
        using rank_id = id_type;
        using card_id = id_type;
        using holdem_id = id_type;

        // random access, means
        //      {id=0, key=4}, {id=2,key=5} needs
        //      {4,-1,5} etc
        template<class T, class Key = id_type>
        struct decl_factory{
                template<class... Args>
                explicit decl_factory(Args&&... args)
                        : vec_{std::forward<Args>(args)...}
                {
                        std::sort( vec_.begin(), vec_.end() );
                }
                T const& get(Key k)const{
                        assert( k < vec_.size() && "key doesn't exist");
                        return vec_[k];
                }
        private:
                std::vector<T> vec_;
        };


        struct suit_decl{
                suit_decl(id_type id, char sym, std::string const& name)
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
                inline static suit_decl const& get(id_type id);
                inline static suit_decl const& get(std::string const& s);
                operator id_type()const{ return id_; }
        private:
                id_type id_;
                char sym_;
                std::string name_;
        };
        
        struct rank_decl{
                rank_decl(id_type id, char sym)
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
                inline static rank_decl const& get(id_type id);
                inline static rank_decl const& get(std::string const& s);
                inline static rank_decl const& get(char c){ return get(std::string{c}); }
                operator id_type()const{ return id_; }
        private:
                id_type id_;
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
                inline static card_decl const& get(id_type id);
                inline static card_decl const& get(std::string const& s){
                        assert( s.size() == 2 && "precondition failed");
                        return get( rank_decl::get(s.substr(0,1)).id() * 4 +
                                    suit_decl::get(s.substr(1,1)).id()   );
                }
                operator id_type()const{ return id_; }
                static id_type make_id( suit_id s, rank_id r){
                        return s + r * 4;
                }
        private:
                id_type id_;
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
                inline static holdem_hand_decl const& get(id_type id);
                inline static holdem_hand_decl const& get(std::string const& s){
                        assert( s.size() == 4 && "precondition failed");
                        auto x = card_decl::get(s.substr(0,2)).id();
                        auto y = card_decl::get(s.substr(2,2)).id();
                        return get(make_id(x,y));
                }
                static id_type make_id( rank_id r0, suit_id s0,
                                        rank_id r1, suit_id s1)
                {
                        id_type x{ card_decl::make_id(s0, r0) };
                        id_type y{ card_decl::make_id(s1, r1) };
                        return make_id(x,y);
                }
                static id_type make_id( card_id x, card_id y){
                        return  x * 52 + y;
                }
                operator id_type()const{ return id_; }
        private:
                id_type id_;
                card_decl first_;
                card_decl second_;

        };

        template<class... Args,
                 class = void_t<
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

        }
                
        suit_decl const& suit_decl::get(id_type id){
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
        
        rank_decl const& rank_decl::get(id_type id){
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
        
        card_decl const& card_decl::get(id_type id){
                using namespace decl;
                static decl_factory<card_decl> fac{
                        _Ah, _Kh, _Qh, _Jh, _Th, _9h, _8h, _7h, _6h, _5h, _4h, _3h, _2h,
                        _Ad, _Kd, _Qd, _Jd, _Td, _9d, _8d, _7d, _6d, _5d, _4d, _3d, _2d,
                        _Ac, _Kc, _Qc, _Jc, _Tc, _9c, _8c, _7c, _6c, _5c, _4c, _3c, _2c,
                        _As, _Ks, _Qs, _Js, _Ts, _9s, _8s, _7s, _6s, _5s, _4s, _3s, _2s
                };
                return fac.get(id);
        }
        
        holdem_hand_decl const& holdem_hand_decl::get(id_type id){
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

        enum class suit_category{
                any_suit,
                suited,
                offsuit
        };
                

        namespace literals{

                inline
                id_type operator""_h(const char* s, size_t sz){
                        return holdem_hand_decl::get(s).id();
                }
        }
                

        // 9 * wins
        // sigma
        // equity
        constexpr size_t computation_size = 11;
        constexpr size_t computation_equity_fixed_prec = 1'000'000;

}

#endif // PS_CORE_CARDS_H
