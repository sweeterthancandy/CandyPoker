#ifndef PS_HOLDEM_FRONTEND_H
#define PS_HOLDEM_FRONTEND_H

#include <regex>
#include <set>
#include <list>
#include <algorithm>

#include <boost/variant.hpp>
#include <boost/variant/multivisitors.hpp>
#include <boost/format.hpp>


#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>

#include <boost/range/algorithm.hpp>

#include "ps/detail/print.h"
#include "ps/detail/visit_combinations.h"


#include "ps/cards.h"

namespace ps{

        namespace frontend{

                struct hand{
                        explicit hand(holdem_id id):id_{id}{}
                        auto get()const{ return id_; }
                        bool operator<(hand const& that)const{
                                return get() < that.get();
                        }
                        bool operator==(hand const& that)const{
                                return get() == that.get();
                        }
                        static int order(){ return 0; }
                        friend std::ostream& operator<<(std::ostream& ostr, hand const& self){
                                return ostr << boost::format("hand{%s}") % holdem_hand_decl::get(self.get());
                        }


                        auto i_am_a_duck__to_hand_vector()const{
                                return std::vector<holdem_id>{id_};
                        }
                private:
                        holdem_id id_;
                };

                struct pocket_pair{
                        explicit pocket_pair(rank_id x):x_{x}{}
                        auto get()const{ return x_; }
                        
                        bool operator<(pocket_pair const& that)const{
                                return get() < that.get();
                        }
                        bool operator==(pocket_pair const& that)const{
                                return get() == that.get();
                        }
                        static int order(){ return 1; }
                        friend std::ostream& operator<<(std::ostream& ostr, pocket_pair const& self){
                                return ostr << boost::format("pocker_pair{%s}") % rank_decl::get(self.get());
                        }
                        
                        auto i_am_a_duck__to_hand_vector()const{
                                using namespace decl;
                                std::vector<holdem_id> result;
                                for( auto i=4;i!=1;){
                                        --i;
                                        for( auto j=i;j!=0;){
                                                --j;
                                                result.emplace_back( holdem_hand_decl::make_id( 
                                                        x_, i,
                                                        x_, j) );
                                        }
                                }
                                return std::move(result);
                        }
                private:
                        rank_id x_;
                };

                template<suit_category Suit_Category, int Order_Decl>
                struct basic_rank_tuple{
                        explicit basic_rank_tuple(rank_id x, rank_id y):x_{x},y_{y}{}
                        auto first()const{ return x_; }
                        auto second()const{ return y_; }

                        bool operator<(basic_rank_tuple const& that)const{
                                if( first() == that.second())
                                        return second() < that.second();
                                return first() < that.first();
                        }
                        bool operator==(basic_rank_tuple const& that)const{
                                return first() == that.first() &&
                                        second() == that.second();
                        }
                        static int order(){ return Order_Decl; }
                        friend std::ostream& operator<<(std::ostream& ostr, basic_rank_tuple const& self){
                                const char* name = [](){
                                        switch(Suit_Category){
                                        case suit_category::any_suit:
                                                return "any_suit";
                                        case suit_category::suited:
                                                return "suited";
                                        case suit_category::offsuit:
                                                return "offsuit";
                                        }
                                }();
                                return ostr << boost::format("%s{%s, %s}") % name % rank_decl::get(self.first()) % rank_decl::get(self.second());
                        }
                        auto i_am_a_duck__to_hand_vector()const{
                                using namespace decl;
                                std::vector<holdem_id> result;
                                assert( x_ != y_ && "invariant failed");
                                switch(Suit_Category){
                                case suit_category::any_suit:
                                        for( auto a : {_c, _d, _s, _h } ){
                                                for( auto b : {_c, _d, _s, _h } ){
                                                        result.emplace_back( holdem_hand_decl::make_id( 
                                                                x_, a,
                                                                y_, b) );
                                                }
                                        }
                                        break;
                                case suit_category::suited:
                                        for( auto a : {_c, _d, _s, _h } ){
                                                result.emplace_back( holdem_hand_decl::make_id( 
                                                        x_, a,
                                                        y_, a) );
                                        }
                                        break;
                                case suit_category::offsuit:
                                        for( auto a : {_c, _d, _s, _h } ){
                                                for( auto b : {_c, _d, _s, _h } ){
                                                        if( a == b ) continue;
                                                        result.emplace_back( holdem_hand_decl::make_id( 
                                                                x_, a,
                                                                y_, b) );
                                                }
                                        }
                                        break;
                                }
                                return std::move(result);
                        }
                private:
                        rank_id x_;
                        rank_id y_;
                };

                using any_suit    = basic_rank_tuple<suit_category::any_suit,2>;

                using offsuit     = basic_rank_tuple<suit_category::offsuit,3>;
                using suited      = basic_rank_tuple<suit_category::suited,4>;

                using primitive_tl = boost::mpl::vector<hand, pocket_pair, offsuit, suited, any_suit>;

                using primitive_t = boost::variant<hand, pocket_pair, offsuit, suited, any_suit>;

                struct plus{
                        explicit plus(primitive_t const& prim):prim_{prim}{}
                        primitive_t const&  get()const{ return prim_; }

                        static int order(){ return 5; }
                        bool operator<(plus const& that)const{
                                return false;
                        }
                        bool operator==(plus const& that)const{
                                return false;
                        }
                private:
                        primitive_t prim_;
                };
                struct interval{
                        explicit interval(primitive_t const& first, primitive_t const& last):
                                first_{first}, last_{last}
                        {}
                        primitive_t const& first()const{ return first_; }
                        primitive_t const& last()const{ return last_; }
                        static int order(){ return 6; }
                        bool operator<(interval const& that)const{
                                return false;
                        }
                        bool operator==(interval const& that)const{
                                return false;
                        }
                private:
                        primitive_t first_;
                        primitive_t last_;
                };

                using sub_range_t = boost::variant<plus, interval, primitive_t>;


                namespace detail{
                        struct debug_print : boost::static_visitor<>{
                                explicit debug_print(std::ostream& ostr):ostr_(&ostr){}

                                void operator()(hand const& prim)const{
                                        *ostr_ << boost::format("hand{%s}") % holdem_hand_decl::get(prim.get());
                                }
                                void operator()(pocket_pair const& prim)const{
                                        *ostr_ << boost::format("pocker_pair{%s}") % rank_decl::get(prim.get());
                                }
                                void operator()(offsuit const& prim)const{
                                        *ostr_ << boost::format("offsuit{%s, %s}") % rank_decl::get(prim.first()) % rank_decl::get(prim.second());
                                }
                                void operator()(suited const& prim)const{
                                        *ostr_ << boost::format("suited{%s, %s}") % rank_decl::get(prim.first()) % rank_decl::get(prim.second());
                                }
                                void operator()(any_suit const& prim)const{
                                        *ostr_ << boost::format("any_suit{%s, %s}") % rank_decl::get(prim.first()) % rank_decl::get(prim.second());
                                }
                                void operator()(plus const& sub)const{
                                        *ostr_ << "plus{";
                                        boost::apply_visitor(*this, sub.get());
                                        *ostr_ << "}";
                                }
                                void operator()(interval const& sub)const{
                                        *ostr_ << "interval{";
                                        boost::apply_visitor(*this, sub.first());
                                        *ostr_ << ",";
                                        boost::apply_visitor(*this, sub.last());
                                        *ostr_ << "}";
                                }
                                void operator()(primitive_t const& sub)const{
                                        boost::apply_visitor(*this, sub);
                                }
                        private:
                                std::ostream* ostr_;
                        };

                        struct operator_left_shift : boost::static_visitor<>{
                                explicit operator_left_shift(std::ostream& ostr):ostr_(&ostr){}

                                void operator()(hand const& prim)const{
                                        *ostr_ << holdem_hand_decl::get(prim.get());
                                }
                                void operator()(pocket_pair const& prim)const{
                                        *ostr_ << rank_decl::get(prim.get()) << rank_decl::get(prim.get());
                                }
                                void operator()(offsuit const& prim)const{
                                        *ostr_ << rank_decl::get(prim.first()) << rank_decl::get(prim.second()) << "o";
                                }
                                void operator()(suited const& prim)const{
                                        *ostr_ << rank_decl::get(prim.first()) << rank_decl::get(prim.second()) << "s";
                                }
                                void operator()(any_suit const& prim)const{
                                        *ostr_ << rank_decl::get(prim.first()) << rank_decl::get(prim.second());
                                }
                                void operator()(plus const& sub)const{
                                        boost::apply_visitor(*this, sub.get());
                                        *ostr_ << "+";
                                }
                                void operator()(interval const& sub)const{
                                        boost::apply_visitor(*this, sub.first());
                                        *ostr_ << "-";
                                        boost::apply_visitor(*this, sub.last());
                                }
                                void operator()(primitive_t const& sub)const{
                                        boost::apply_visitor(*this, sub);
                                }
                        private:
                                std::ostream* ostr_;
                        };

                        struct to_set : boost::static_visitor<>{
                                explicit to_set(std::set<holdem_id>& s):s_{&s}{}
                                void operator()(hand const& prim)const{
                                        s_->insert(prim.get());
                                }
                                void operator()(pocket_pair const& prim)const{
                                }
                                void operator()(offsuit const& prim)const{
                                }
                                void operator()(suited const& prim)const{
                                        for( auto s : { 0,1,2,3} ){
                                                s_->insert( holdem_hand_decl::make_id(
                                                        prim.first(), s,
                                                        prim.second(), s) );
                                        }
                                }
                                void operator()(any_suit const& prim)const{
                                }
                                void operator()(plus const& sub)const{
                                }
                                void operator()(interval const& sub)const{
                                }
                                void operator()(primitive_t const& sub)const{
                                        boost::apply_visitor(*this, sub);
                                }
                        private:
                                std::set<holdem_id>* s_;
                        };
                        
                        
                        struct plus_expander : boost::static_visitor<>{
                                explicit plus_expander(std::vector<sub_range_t>& vec):vec_{&vec}{}

                                void operator()(hand const& prim)const{
                                        assert( "not implemtned");
                                }
                                void operator()(pocket_pair const& prim)const{
                                        for( rank_id r{prim.get()}; r <= decl::_A; ++r){
                                                vec_->emplace_back( pocket_pair{r} );
                                        }
                                }
                                template<class T,
                                        class = std::enable_if_t<  boost::mpl::contains<boost::mpl::vector<offsuit, suited, any_suit>, std::decay_t<T> >::value>
                                >
                                void operator()(T const& prim)const{
                                        rank_id x{ prim.first() };
                                        rank_id y{ prim.second() };
                                        assert( x > y && "unexpected");
                                        for( rank_id r{y}; r < x; ++r){
                                                vec_->emplace_back( T{x,r} );
                                        }
                                }
                        private:
                                std::vector<sub_range_t>* vec_;
                        };
                        struct interval_expander : boost::static_visitor<>{
                                explicit interval_expander(std::vector<sub_range_t>& vec):vec_{&vec}{}

                                void operator()(pocket_pair const& first, pocket_pair const& last)const{
                                        rank_id x{ std::min ( first.get(), last.get() ) };
                                        rank_id y{ std::max ( first.get(), last.get() ) };
                                        for( rank_id r{x}; r <= y; ++r){
                                                vec_->emplace_back( pocket_pair{r} );
                                        }
                                }
                                template<class T,
                                        class = std::enable_if_t<  boost::mpl::contains<boost::mpl::vector<offsuit, suited, any_suit>, std::decay_t<T> >::value>
                                >
                                void operator()(T const& first, T const& last)const{
                                        // A5s -> A2s
                                        auto first_x{ first.first() };
                                        auto first_y{ first.second() };
                                        auto last_x{ last.first() };
                                        auto last_y{ last.second() };
                                        if( first_x != last_x )
                                                BOOST_THROW_EXCEPTION(std::domain_error("bad range "));
                                        for( rank_id r{std::min(first_y, last_y)}; r <= std::max(first_y, last_y); ++r){
                                                if( r == first_x ) continue;
                                                vec_->emplace_back( T{first_x,r} );
                                        }

                                }
                                template<class T, class U>
                                void operator()(T const first, U const& second)const{
                                        BOOST_THROW_EXCEPTION( std::domain_error("can't have hetrogenous interval"));
                                }
                        private:
                                std::vector<sub_range_t>* vec_;
                        };
                        
                        struct expander : boost::static_visitor<>{
                                explicit expander(std::vector<sub_range_t>& vec):vec_{&vec},plus_{vec},interval_{vec}{}

                                void operator()(plus const& sub)const{
                                        boost::apply_visitor( plus_, sub.get() );
                                }
                                void operator()(interval const& sub)const{
                                        boost::apply_visitor( interval_, sub.first(), sub.last() );
                                }
                                void operator()(primitive_t const& sub)const{
                                        // want to expand any_suit
                                        boost::apply_visitor( *this, sub);
                                }
                                
                                
                                void operator()(hand const& prim)const{
                                        vec_->emplace_back(prim);
                                }
                                void operator()(pocket_pair const& prim)const{
                                        vec_->emplace_back(prim);
                                }
                                void operator()(offsuit const& prim)const{
                                        vec_->emplace_back(prim);
                                }
                                void operator()(suited const& prim)const{
                                        vec_->emplace_back(prim);
                                }
                                void operator()(any_suit const& prim)const{
                                        vec_->emplace_back( suited{ prim.first(), prim.second() } );
                                        vec_->emplace_back( offsuit{ prim.first(), prim.second() } );
                                }

                                
                        private:
                                std::vector<sub_range_t>* vec_;
                                plus_expander plus_;
                                interval_expander interval_;
                        };

                        
                        struct sorter : boost::static_visitor<bool>{
                                template<class T >
                                bool operator()(T const& first, T const& last)const{
                                        return first < last;
                                }
                                template<class T, class U>
                                void operator()(T const first, U const& second)const{
                                        return T::order() < T::order();
                                }
                        };

                        struct primitive_cast : boost::static_visitor<primitive_t>{
                                primitive_t operator()(plus const& )const{
                                        BOOST_THROW_EXCEPTION(std::domain_error("not a prim"));
                                }
                                primitive_t operator()(interval const& )const{
                                        BOOST_THROW_EXCEPTION(std::domain_error("not a prim"));
                                }
                                primitive_t operator()(primitive_t const& prim)const{
                                        return prim;
                                }

                                // should really need this
                                template<class T,
                                        class = std::enable_if_t<  boost::mpl::contains<boost::mpl::vector<offsuit, suited, any_suit>, std::decay_t<T> >::value>
                                >
                                primitive_t operator()(T const& prim)const{
                                        return prim;
                                }
                        };

                        struct prim_equal : boost::static_visitor<bool>{
                                template<class T >
                                bool operator()(T const& first, T const& second)const{
                                        return first == second;
                                }
                                template<class T, class U>
                                bool operator()(T const first, U const& second)const{
                                        return false;
                                }
                        };

                        struct to_hand_vector : boost::static_visitor<std::vector<holdem_id> >{
                                template<class T>
                                std::vector<holdem_id> operator()(T const& val)const{
                                        return val.i_am_a_duck__to_hand_vector();
                                }
                        };

                }

                struct primitive_range{
                        explicit primitive_range(std::vector<primitive_t> const& vec):
                                vec_{vec}
                        {}
                        friend std::ostream& operator<<(std::ostream& ostr, primitive_range const& self){
                                using printer = detail::debug_print;
                                for(size_t i{0};i!=self.vec_.size();++i){
                                        if( i != 0)
                                                ostr << ", ";
                                        boost::apply_visitor( printer(ostr), self.vec_[i] );
                                }
                                return ostr;
                        }
                        primitive_range& operator+=(primitive_t const& prim){
                                vec_.emplace_back(prim);
                                return *this;
                        }
                        primitive_t const& operator[](size_t idx)const{
                                return vec_[idx];
                        }
                        decltype(auto) size()const{ return vec_.size(); }
                        decltype(auto) begin()const{ return vec_.begin(); }
                        decltype(auto) end()const{ return vec_.end(); }
                private:
                        std::vector<primitive_t> vec_;
                };

                struct range{
                        friend std::ostream& operator<<(std::ostream& ostr, range const& self){
                                using printer = detail::debug_print;
                                for(size_t i{0};i!=self.subs_.size();++i){
                                        if( i != 0)
                                                ostr << ", ";
                                        boost::apply_visitor( printer(ostr), self.subs_[i] );
                                }
                                return ostr;
                        }
                        range& operator+=(sub_range_t sub){
                                subs_.emplace_back(sub);
                                return *this;
                        }

                        /*
                         * This function maps the range structore to a subset,
                         * so that seach sub range is exclusive, and 
                         * plus and intervals are mapped to primtives
                         */
                        friend range expand(range const& self){
                                std::vector<sub_range_t> vec;
                                detail::expander aux{vec};
                                boost::for_each( self.subs_, boost::apply_visitor(aux) );
                                if( vec.empty() ){
                                        return range{};
                                }
                                boost::sort( vec, detail::sorter() );
                                std::list<primitive_t> subs;
                                for( auto const& e : vec){
                                        subs.emplace_back( boost::apply_visitor(detail::primitive_cast(), e));
                                }
                                // now want to remove duplicates, and also remove
                                // AhKh when there is AKs or AK etc
                                
                                std::vector<decltype(subs.begin())> to_remove;
                                auto head = subs.begin();
                                auto iter = head;
                                ++iter;
                                auto end = subs.end();
                                for(; iter != end;){
                                        if( boost::apply_visitor( detail::prim_equal(), *head, *iter) ){
                                                to_remove.emplace_back(iter);
                                                ++iter;
                                        }else{
                                                head = iter;
                                                ++iter;
                                        }
                                }
                                for( auto iter : to_remove )
                                        subs.erase(iter);
                                
                                range result;
                                boost::copy( subs, std::back_inserter(result.subs_));
                                return std::move(result);
                        }
                        auto to_primitive_range()const{
                                std::vector<primitive_t> vec;
                                for( auto const& e : subs_){
                                        vec.emplace_back( boost::apply_visitor(detail::primitive_cast(), e));
                                }
                                return primitive_range{std::move(vec)};
                        }
                private:
                        std::vector<sub_range_t> subs_;
                };


                template<class T, 
                        class = std::enable_if_t< boost::mpl::contains<primitive_tl, std::decay_t<T> >::value>
                >
                inline auto operator++(T const& val, int){
                        return plus{val};
                }
                template<class T, class U, 
                        class = std::enable_if_t< boost::mpl::contains<primitive_tl, std::decay_t<T> >::value>,
                        class = std::enable_if_t< boost::mpl::contains<primitive_tl, std::decay_t<U> >::value>
                >
                inline auto operator-(T const& first, U const& last){
                        return interval{first, last};
                }
                


                namespace detail{
                        template<class Iter_Type>
                        struct parser{
                                range parse(Iter_Type start, Iter_Type end){
                                        restart();

                                        begin = start;
                                        iter = begin;
                                        last = end;
                                                
                                        // eat whitespace
                                        for(;iter!=last && std::isspace(*iter);++iter);

                                        for(;iter != last;){

                                                if( ! parse_sub_range_() )
                                                        error_(std::domain_error("bad syntax for (" + std::string(begin,last) + ")"));


                                                if( iter == last )
                                                        break;
                                                
                                                // seperator
                                                switch(*iter){
                                                case ',':
                                                case ';':
                                                        ++iter;
                                                        continue;
                                                default:
                                                        if( std::isspace(*iter)){
                                                                eat_ws_();
                                                                continue;
                                                        }
                                                }
                                                error_(std::domain_error("baad syntax for (" + std::string(begin,last) + ")"));
                                        }

                                        return working_;
                                }
                                template<class E>
                                void error_(E const& e){
                                        BOOST_THROW_EXCEPTION(e);
                                }
                                bool parse_sub_range_(){
                                        if( ! parse_prim_() )
                                                return false;
                                        if( iter != last){

                                                if( *iter == '+'){
                                                        ++iter;
                                                        working_ += plus(stack_.back());
                                                        stack_.pop_back();
                                                        return true;
                                                }
                                                if( *iter == '-'){
                                                        ++iter;
                                                        if( ! parse_prim_() )
                                                                return false;
                                                        working_ += interval(stack_[stack_.size()-2], stack_[stack_.size()-1]);
                                                        stack_.pop_back();
                                                        stack_.pop_back();
                                                        return true;
                                                }
                                        }
                                        working_ += stack_.back();
                                        stack_.pop_back();
                                        return true;
                                }
                                // eat whitespace
                                void eat_ws_(){
                                        for(;iter!=last && std::isspace(*iter);++iter);
                                }
                                void restart(){
                                        stack_.clear();
                                        working_.~range();
                                        new(&working_)range;
                                }
                                bool parse_prim_(){
                                        eat_ws_();
                                        std::smatch m;

                                        //std::cout << "parse_prim_(" << std::string(iter,last) << ", [stack_.size()=" << stack_.size() << "])" << std::endl;

                                        if( std::regex_search( iter, last, m, rgx_XX_suit_decl ) ){
                                                switch(m[0].str()[2]){
                                                case 'o': case 'O':
                                                        stack_.emplace_back( offsuit{rank_decl::get(*iter), rank_decl::get(*(iter+1))});
                                                        break;
                                                case 's': case 'S':
                                                        stack_.emplace_back( suited{rank_decl::get(*iter), rank_decl::get(*(iter+1))});
                                                        break;
                                                }
                                                iter += m[0].length();
                                                return true;
                                        }
                                        if( std::regex_search( iter, last, m, rgx_XX) ){
                                                if( m[0].str()[0] == m[0].str()[1] ){
                                                        stack_.emplace_back( pocket_pair{rank_decl::get(*iter)});
                                                } else{
                                                        stack_.emplace_back( any_suit{rank_decl::get(*iter), rank_decl::get(*(iter+1))});
                                                }
                                                iter += m[0].length();
                                                return true;
                                        }
                                        // AX+, Ax
                                        if( std::regex_search( iter, last, m, rgx_X_) ){
                                                stack_.emplace_back( any_suit{rank_decl::get(*iter), rank_decl::get('2')});;
                                                iter += m[0].length();
                                                return true;
                                        }
                                        return false;
                                }
                        private:
                                Iter_Type iter;
                                Iter_Type begin;
                                Iter_Type last;
                                std::vector<primitive_t> stack_;
                                range working_;

                                
                                std::regex rgx_XX_suit_decl{"^[AaKkJjQqTt[:digit:]]{2}[oOsS]"};
                                std::regex rgx_XX{"^[AaKkJjQqTt[:digit:]]{2}"};
                                std::regex rgx_X_{"^[AaKkJjQqTt[:digit:]][xX]"};
                        };
                }

                inline
                range parse(std::string const& str){
                        static detail::parser<decltype(str.begin())> p;
                        return p.parse(str.begin(), str.end());
                }

                template<class T>
                auto to_hand_vector(T const& val)->decltype( val.i_am_a_duck__to_hand_vector()){
                        return val.i_am_a_duck__to_hand_vector();
                }
                inline
                auto to_hand_vector(primitive_t const& prim){
                        return boost::apply_visitor( detail::to_hand_vector(), prim);
                }


                #if 0
                template<class T,
                        class = std::enable_if_t<
                                boost::mpl::contains<
                                        boost::mpl::vector<hand, pocket_pair, offsuit, suited, any_suit,
                                                           interval, plus, primitive_t>, std::decay_t<T>>::value>
                >
                std::ostream& operator<<(std::ostream& ostr, T const& val){
                        using printer = detail::debug_print;
                        boost::apply_visitor( printer(ostr), val);
                        return ostr;
                }
                #endif
        }


}

#include "ps/frontend_decl.h"

#endif // PS_HOLDEM_FRONTEND_H
