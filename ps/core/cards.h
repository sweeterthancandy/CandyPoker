#ifndef PS_CORE_CARDS_H
#define PS_CORE_CARDS_H

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

namespace ps{

        template<class T, class Key = char>
        struct decl_factory{
                template<class... Args>
                explicit decl_factory(Args&&... args)
                        : vec_{std::forward<Args>(args)...}
                {
                        std::sort( vec_.begin(), vec_.end() );
                }
                T const& get(Key k)const{
                        return vec_[k];
                }
        private:
                std::vector<T> vec_;
        };


        struct suit_decl{
                suit_decl(char id, char sym, std::string const& name)
                        : id_{id}, sym_{sym}, name_{name}
                {}
                char id()const{ return id_; }
                std::string to_string()const{ return std::string{sym_}; }
                friend std::ostream& operator<<(std::ostream& ostr, suit_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(suit_decl const& that){
                        return id_ < that.id_;
                }
                inline static suit_decl const& get(char id);
        private:
                char id_;
                char sym_;
                std::string name_;
        };
        
        struct rank_decl{
                rank_decl(char id, char sym)
                        : id_{id}, sym_{sym}
                {}
                char id()const{ return id_; }
                std::string to_string()const{ return std::string{sym_}; }
                friend std::ostream& operator<<(std::ostream& ostr, rank_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(rank_decl const& that){
                        return id_ < that.id_;
                }
                inline static rank_decl const& get(char id);
        private:
                char id_;
                char sym_;
        };

        struct card_decl{
                card_decl( suit_decl s, rank_decl r):
                        suit_{s}, rank_{r}
                , id_{static_cast<char>(suit_.id() + rank_.id() * 4)}
                {}
                std::string to_string()const{
                        return rank_.to_string() + 
                               suit_.to_string();
                }
                suit_decl const& suit()const{ return suit_; }
                rank_decl const& rank()const{ return rank_; }
                friend std::ostream& operator<<(std::ostream& ostr, card_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(card_decl const& that){
                        return id_ < that.id_;
                }
                inline static card_decl const& get(char id);
        private:
                suit_decl suit_;
                rank_decl rank_;
                char id_;
        };
        
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
                
        suit_decl const& suit_decl::get(char id){
                using namespace decl;
                static decl_factory<suit_decl> fac{_h, _d, _c, _s};
                return fac.get(id);
        }
        
        rank_decl const& rank_decl::get(char id){
                using namespace decl;
                static decl_factory<rank_decl> fac{_2,_3,_4,_5,_6,
                                                   _7,_9,_9,_T,_J,
                                                   _Q,_K,_A};
                return fac.get(id);
        }
        
        card_decl const& card_decl::get(char id){
                using namespace decl;
                static decl_factory<card_decl> fac{
                        _Ah, _Kh, _Qh, _Jh, _Th, _9h, _8h, _7h, _6h, _5h, _4h, _3h, _2h,
                        _Ad, _Kd, _Qd, _Jd, _Td, _9d, _8d, _7d, _6d, _5d, _4d, _3d, _2d,
                        _Ac, _Kc, _Qc, _Jc, _Tc, _9c, _8c, _7c, _6c, _5c, _4c, _3c, _2c,
                        _As, _Ks, _Qs, _Js, _Ts, _9s, _8s, _7s, _6s, _5s, _4s, _3s, _2s
                };
                return fac.get(id);
        }

                

}

#endif // PS_CORE_CARDS_H
