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
#include "ps/base/cards.h"
#include "ps/base/algorithm.h"
#include "ps/support/singleton_factory.h"
#include "ps/base/frontend.h"
#include "ps/detail/cross_product.h"
#include <iostream>

#include <boost/range/algorithm.hpp>



namespace{
        // random access, means
        //      {id=0, key=4}, {id=2,key=5} needs
        //      {4,-1,5} etc
        template<class T, class Key = ps::id_type>
        struct decl_factory{
                template<class... Args>
                explicit decl_factory(Args&&... args)
                        : vec_{std::forward<Args>(args)...}
                {
                        boost::sort( vec_ );
                }
                T const& get(Key k)const{
                        assert( k < vec_.size() && "precondition failed, key doesn't exist");
                        return vec_[k];
                }
        private:
                std::vector<T> vec_;
        };
}

namespace ps{
        suit_decl const& suit_decl::get(suit_id id){
                using namespace decl;
                static decl_factory<suit_decl> fac{_h, _d, _c, _s};
                return fac.get(id);
        }
        suit_decl const& suit_decl::parse(std::string const& s){
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
        rank_decl const& rank_decl::parse(std::string const& s){
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
        holdem_hand_decl const& holdem_hand_decl::parse(std::string const& s){
                assert( s.size() == 4 && "precondition failed");
                auto x = card_decl::parse(s.substr(0,2)).id();
                auto y = card_decl::parse(s.substr(2,2)).id();
                return get(make_id(x,y));
        }
        holdem_id holdem_hand_decl::make_id( rank_id r0, suit_id s0,
                                rank_id r1, suit_id s1)
        {
                holdem_id x{ card_decl::make_id(s0, r0) };
                holdem_id y{ card_decl::make_id(s1, r1) };
                return make_id(x,y);
        }
        
        holdem_hand_decl const& holdem_hand_decl::get(holdem_id id){
                static decl_factory<holdem_hand_decl> fac{
                        [](){
                                std::vector< holdem_hand_decl> aux;
                                for( char a{52};a!=1;){
                                        --a;
                                        for( char b{a};b!=0;){
                                                --b;
                                                aux.emplace_back( card_decl::get(a), card_decl::get(b));
                                        }
                                }
                                return std::move(aux);
                        }()
                };
                return fac.get(id);
        }
        double holdem_hand_decl::prob(holdem_id c0, holdem_id c1){
                // static
                static auto aux = [](){
                        size_t count{0};
                        for(size_t i{0};i!=holdem_hand_decl::max_id;++i){
                                for(size_t j{0};j!=holdem_hand_decl::max_id;++j){
                                        if( disjoint(holdem_hand_decl::get(i),
                                                     holdem_hand_decl::get(j) ) )
                                                ++count;
                                }
                        }
                        return count;
                }();
                return 
                        1.0 / aux;
        }

        holdem_class_id holdem_class_decl::make_id(holdem_class_type cat, rank_id x, rank_id y){
                using std::get;
                static auto aux = [](){
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
                        boost::sort(vec);
                        return std::move(vec);
                }();
                if( x < y )
                        std::swap(x,y);
                 
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
                                for( rank_id a{13};a!=0;){
                                        --a;
                                        aux.emplace_back( holdem_class_type::pocket_pair,
                                                          rank_decl::get(a),
                                                          rank_decl::get(a) );
                                }
                                for( rank_id a{13};a!=1;){
                                        --a;
                                        for( rank_id b{a};b!=0;){
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


                
                
        holdem_class_decl::holdem_class_decl(holdem_class_type cat,
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
                                for(suit_id y{static_cast<suit_id>(x+1)};y!=4;++y){
                                        hand_set_.emplace_back(
                                                card_decl(
                                                        suit_decl::get(x), first_),
                                                card_decl(
                                                        suit_decl::get(y), first_));
                                }
                        }
                        break;
                }
                for( auto const& d : hand_set_ ){
                        hand_id_set_.emplace_back(d.id());
                }
        }

        std::string holdem_class_decl::to_string()const{
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
        holdem_class_decl const& holdem_class_decl::parse(std::string const& s){
                auto x = rank_decl::parse(s.substr(0,1)).id();
                auto y = rank_decl::parse(s.substr(1,1)).id();
                //PRINT_SEQ((s)(x)(y));
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
        size_t holdem_class_decl::weight(holdem_class_id c0, holdem_class_id c1){
                auto const& left =  holdem_class_decl::get(c0).get_hand_set() ;
                auto const& right =  holdem_class_decl::get(c1).get_hand_set() ;
                size_t count{0};
                for( auto const& l : left ){
                        for( auto const& r : right ){
                                if( disjoint(l, r) ){
                                        ++count;
                                }
                        }
                }
                return count;
        }
        double holdem_class_decl::prob(holdem_class_id c0, holdem_class_id c1){
                // I just got this magic constant my summing all the weights
                static const size_t factor = 
                        [](){
                                size_t ret{0};
                                for(size_t i{0};i!=holdem_class_decl::max_id;++i){
                                        for(size_t j{0};j!=holdem_class_decl::max_id;++j){
                                                ret += holdem_class_decl::weight(i,j);
                                        }
                                }
                                return ret;
                        }();
                return static_cast<double>(weight(c0, c1)) / factor;
        }

        holdem_class_id holdem_hand_decl::class_()const{
                holdem_class_type type;
                if( first().rank() == second().rank() ){
                        type = holdem_class_type::pocket_pair;
                } else if ( first().suit() == second().suit() ){
                        type = holdem_class_type::suited;
                } else{
                        type = holdem_class_type::offsuit;
                }
                return holdem_class_decl::make_id(type, first().rank().id(), second().rank().id());
        }
        
        
        holdem_class_range::holdem_class_range(std::string const& item){
                this->parse(item);
        }

        std::ostream& operator<<(std::ostream& ostr, holdem_class_range const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_class_decl::get(id).to_string();
                                                 } );
        }
        void holdem_class_range::parse(std::string const& item){
                auto rep = expand(frontend::parse(item));
                boost::copy( rep.to_class_vector(), std::back_inserter(*this)); 
        }



        std::vector<holdem_class_vector> holdem_class_range_vector::get_cross_product()const{
                std::vector<holdem_class_vector> ret;
                detail::cross_product_vec([&](auto const& byclass){
                        ret.emplace_back();
                        for( auto iter : byclass ){
                                ret.back().emplace_back(*iter);
                        }
                }, *this);
                return std::move(ret);
        }

        std::vector<
               std::tuple< std::vector<int>, holdem_hand_vector >
        > holdem_class_range_vector::to_standard_form()const{

                auto const n = size();

                std::map<holdem_hand_vector, std::vector<int>  > result;

                for( auto const& cv : get_cross_product()){
                        for( auto hv : cv.get_hand_vectors()){

                                auto p =  permutate_for_the_better(hv) ;
                                auto& perm = std::get<0>(p);
                                auto const& perm_players = std::get<1>(p);

                                if( result.count(perm_players) == 0 ){
                                        result[perm_players].resize(n*n);
                                }
                                auto& item = result.find(perm_players)->second;
                                for(int i=0;i!=n;++i){
                                        ++item[i*n + perm[i]];
                                }
                        }
                }
                std::vector< std::tuple< std::vector<int>, holdem_hand_vector > > ret;
                for( auto& m : result ){
                        ret.emplace_back( std::move(m.second), std::move(m.first));
                }
                return std::move(ret);
        }
        std::vector<
               std::tuple< std::vector<int>, holdem_class_vector >
        > holdem_class_range_vector::to_class_standard_form()const{

                auto const n = size();

                std::map<holdem_class_vector, std::vector<int>  > result;

                for( auto const& cv : get_cross_product()){

                        auto stdform = cv.to_standard_form();
                        auto const& perm = std::get<0>(stdform);
                        auto const& perm_cv = std::get<1>(stdform);

                        if( result.count(perm_cv) == 0 ){
                                result[perm_cv].resize(n*n);
                        }
                        auto& item = result.find(perm_cv)->second;
                        for(int i=0;i!=n;++i){
                                ++item[i*n + perm[i]];
                        }
                }
                std::vector< std::tuple< std::vector<int>, holdem_class_vector > > ret;
                for( auto& m : result ){
                        ret.emplace_back( std::move(m.second), std::move(m.first));
                }
                return std::move(ret);
        }
                        
        std::ostream& operator<<(std::ostream& ostr, holdem_class_range_vector const& self){
                return ostr << detail::to_string(self);
        }

        void holdem_class_range_vector::push_back(std::string const& s){
                this->emplace_back(s);
        }



        std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_class_decl::get(id).to_string();
                                                 } );
        }
        holdem_class_decl const& holdem_class_vector::decl_at(size_t i)const{
                return holdem_class_decl::get( 
                        this->operator[](i)
                );
        }

        std::vector< holdem_hand_vector > holdem_class_vector::get_hand_vectors()const{
                std::vector< holdem_hand_vector > stack;
                stack.emplace_back();

                for(size_t i=0; i!= this->size(); ++i){
                        decltype(stack) next_stack;
                        auto const& hand_set =  this->decl_at(i).get_hand_set() ;
                        for( size_t j=0;j!=hand_set.size(); ++j){
                                for(size_t k=0;k!=stack.size();++k){
                                        next_stack.push_back( stack[k] );
                                        next_stack.back().push_back( hand_set[j].id() );
                                        if( ! next_stack.back().disjoint() )
                                                next_stack.pop_back();
                                }
                        }
                        stack = std::move(next_stack);
                }
                return std::move(stack);
        }
        std::tuple<
                std::vector<int>,
                holdem_class_vector
        > holdem_class_vector::to_standard_form()const{
                std::vector<std::tuple<holdem_class_id, int> > aux;
                for(int i=0;i!=size();++i){
                        aux.emplace_back( (*this)[i], i);
                }
                boost::sort( aux, [](auto const& l, auto const& r){
                        return std::get<0>(l) < std::get<0>(r);
                });
                std::vector<int> perm;
                holdem_class_vector vec;
                for( auto const& t : aux){
                        vec.push_back(std::get<0>(t));
                        perm.push_back( std::get<1>(t));
                }
                return std::make_tuple(
                        std::move(perm),
                        std::move(vec)
                );
        }
        
        std::vector<
               std::tuple< std::vector<int>, holdem_hand_vector >
        > holdem_class_vector::to_standard_form_hands()const{
                auto const n = size();

                std::map<holdem_hand_vector, std::vector<int>  > result;

                for( auto hv : get_hand_vectors()){

                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);

                        if( result.count(perm_players) == 0 ){
                                result[perm_players].resize(n*n);
                        }
                        auto& item = result.find(perm_players)->second;
                        for(int i=0;i!=n;++i){
                                ++item[i*n + perm[i]];
                        }
                }
                std::vector< std::tuple< std::vector<int>, holdem_hand_vector > > ret;
                for( auto& m : result ){
                        ret.emplace_back( std::move(m.second), std::move(m.first));
                }
                return std::move(ret);
        }

        bool holdem_class_vector::is_standard_form()const{
                for( size_t idx = 1; idx < size();++idx){
                        if( (*this)[idx-1] > (*this)[idx] )
                                return false; 
                }
                return true;
        }

        holdem_hand_decl const& holdem_hand_vector::decl_at(size_t i)const{
                return holdem_hand_decl::get( 
                        this->operator[](i)
                );
        }
        std::ostream& operator<<(std::ostream& ostr, holdem_hand_vector const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_hand_decl::get(id).to_string();
                                                 } );
        }
        auto holdem_hand_vector::find_injective_permutation()const{
                auto tmp =  permutate_for_the_better(*this) ;
                return std::make_tuple(
                        std::get<0>(tmp),
                        holdem_hand_vector(std::move(std::get<1>(tmp))));
        }
        bool holdem_hand_vector::disjoint()const{
                std::set<card_id> s;
                for( auto id : *this ){
                        auto const& decl = holdem_hand_decl::get(id);
                        s.insert( decl.first() );
                        s.insert( decl.second() );
                }
                return s.size() == this->size()*2;
        }

        bool holdem_hand_vector::is_standard_form()const{
                auto p =  permutate_for_the_better(*this);
                auto const& perm = std::get<0>(p);
                // TODO, need to make sure AA KK KK QQ persevers order etc
                for( int i=0;i!=perm.size();++i){
                        if( perm[i] != i )
                                return false;
                }
                return true;
        }
        card_vector holdem_hand_vector::to_card_vector()const{
                card_vector vec;
                for( auto id : *this ){
                        auto const& hand = holdem_hand_decl::get(id);
                        vec.push_back(hand.first().id());
                        vec.push_back(hand.second().id());
                }
                return std::move(vec);
        }
        size_t holdem_hand_vector::mask()const{
                size_t m = 0;
                for( auto id : *this ){
                        auto const& hand = holdem_hand_decl::get(id);
                        m |= (static_cast<size_t>(1) << hand.first().id() );
                        m |= (static_cast<size_t>(1) << hand.second().id() );
                }
                return m;
        }
        
        std::ostream& operator<<(std::ostream& ostr, rank_vector const& self){
                return ostr << detail::to_string(self, [](auto id){
                                                 return rank_decl::get(id).to_string();
                });
        }

} // 
