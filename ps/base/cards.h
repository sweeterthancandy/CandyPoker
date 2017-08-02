#ifndef PS_CORE_CARDS_H
#define PS_CORE_CARDS_H

#include <vector>
#include <set>


#include "ps/base/cards_fwd.h"
#include "ps/detail/void_t.h"
#include "ps/detail/print.h"


namespace ps{

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
                static constexpr const holdem_id max_id = 52 * 51 / 2;
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
                static holdem_hand_decl const& get(card_id x, card_id y){
                        return get( make_id(x,y) );
                }
                static holdem_hand_decl const& parse(std::string const& s);
                static holdem_id make_id( rank_id r0, suit_id s0, rank_id r1, suit_id s1);
                inline static holdem_id make_id( card_id x, card_id y){
                        static std::array<holdem_id, 52*52> proto{
                                [](){
                                        size_t id{0};
                                        std::array<holdem_id, 52*52> result;
                                        for( char a{52};a!=1;){
                                                --a;
                                                for( char b{a};b!=0;){
                                                        --b;
                                                        result[a * 52 + b ] = id;
                                                        result[b * 52 + a ] = id;
                                                        ++id;
                                                }
                                        }
                                        //PRINT_SEQ((id)((52*52-52)/2));
                                        return std::move(result);
                                }()
                        };
                        assert( x < 52 && "precondition failed");
                        assert( y < 52 && "precondition failed");
                        return proto[x * 52 + y];
                }
                operator holdem_id()const{ return id_; }
                holdem_class_id class_()const;
                static double prob(holdem_id c0, holdem_id c1);
        private:
                holdem_id id_;
                card_decl first_;
                card_decl second_;

        };

        struct holdem_class_decl{
                static constexpr const holdem_class_id max_id = 13 * 13;
                holdem_class_decl(holdem_class_type cat,
                                  rank_decl const& a,
                                  rank_decl const& b);
                auto const& get_hand_set()const{ return hand_set_; }
                auto id()const{ return id_; }
                auto category()const{ return cat_; }
                std::string to_string()const;
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(holdem_class_decl const& that)const{
                        return id_ < that.id_;
                }
                size_t weight()const{
                        switch(cat_){
                        case holdem_class_type::suited:
                                return 4;
                        case holdem_class_type::offsuit:
                                return 12;
                        case holdem_class_type::pocket_pair:
                                return 6;
                        }
                }
                double prob()const{
                        constexpr size_t half{ ( 13 * 13 - 13 ) / 2 };
                        constexpr size_t sigma{ 
                                13   *  6 +
                                half * 12 +
                                half * 4
                        };
                        return static_cast<double>(weight()) / sigma;
                }
                decltype(auto) first()const{ return first_; }
                decltype(auto) second()const{ return second_; }
                static holdem_class_decl const& get(holdem_id id);
                static holdem_class_decl const& parse(std::string const& s);
                // any bijection will do, nice to keep the mapping within a char
                static holdem_class_id make_id(holdem_class_type cat, rank_id x, rank_id y);
                operator holdem_class_id()const{ return id_; }

                static size_t weight(holdem_class_id c0, holdem_class_id c1);
                static double prob(holdem_class_id c0, holdem_class_id c1);
        private:
                holdem_class_id id_;
                holdem_class_type cat_;
                rank_decl first_;
                rank_decl second_;
                std::vector<holdem_hand_decl> hand_set_;
                std::vector<holdem_id> hand_id_vec_;
        };


                

}

#include "ps/base/decl.h"

#endif // PS_CORE_CARDS_H
