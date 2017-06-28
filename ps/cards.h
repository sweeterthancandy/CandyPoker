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
        using id_type         =  std::uint16_t;

        using suit_id         = std::uint8_t;
        using rank_id         = std::uint8_t; 
        using card_id         = std::uint8_t; 
        using holdem_id       = std::uint16_t;
        using holdem_class_id = std::uint8_t; 
        #else

        using id_type =  unsigned;

        using suit_id   = unsigned;
        using rank_id   = unsigned;
        using card_id   = unsigned;
        using holdem_id = unsigned;
        using holdem_class_id = unsigned;

        #endif


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
                static suit_decl const& get(suit_id id);
                static suit_decl const& parse(std::string const& s);
                static suit_decl const& parse(char c){ return parse(std::string{c}); }
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
                static rank_decl const& get(rank_id id);
                static rank_decl const& parse(std::string const& s);
                static rank_decl const& parse(char c){ return parse(std::string{c}); }
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
                static card_decl const& get(card_id id);
                inline static card_decl const& parse(std::string const& s){
                        assert( s.size() == 2 && "precondition failed");
                        return get( rank_decl::parse(s.substr(0,1)).id() * 4 +
                                    suit_decl::parse(s.substr(1,1)).id()   );
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
                static holdem_hand_decl const& get(holdem_id id);
                static holdem_hand_decl const& parse(std::string const& s);
                static holdem_id make_id( rank_id r0, suit_id s0, rank_id r1, suit_id s1);
                inline static holdem_id make_id( card_id x, card_id y){
                        return  x * 52 + y;
                }
                operator holdem_id()const{ return id_; }
        private:
                holdem_id id_;
                card_decl first_;
                card_decl second_;

        };

        struct holdem_class_decl{
                holdem_class_decl(holdem_class_type cat,
                                  rank_decl const& a,
                                  rank_decl const& b);
                auto get_hand_set()const{ return hand_set_; }
                auto id()const{ return id_; }
                auto category(){ return cat_; }
                std::string to_string()const;
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(holdem_class_decl const& that)const{
                        return id_ < that.id_;
                }
                decltype(auto) first()const{ return first_; }
                decltype(auto) second()const{ return second_; }
                static holdem_class_decl const& get(holdem_id id);
                static holdem_class_decl const& parse(std::string const& s);
                // any bijection will do, nice to keep the mapping within a char
                static holdem_class_id make_id(holdem_class_type cat, rank_id x, rank_id y);
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

                

}

#include "ps/decl.h"

#endif // PS_CORE_CARDS_H
