#ifndef PS_HOLDEM_FRONTEND_H
#define PS_HOLDEM_FRONTEND_H

#include <boost/variant.hpp>


#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>

#include <regex>

#include "ps/core/cards.h"

namespace ps{

        namespace frontend{

                struct hand{
                        explicit hand(holdem_id id):id_{id}{}
                        auto get()const{ return id_; }
                private:
                        holdem_id id_;
                };

                struct pocket_pair{
                        explicit pocket_pair(rank_id x):x_{x},y_{x}{}
                        auto first()const{ return x_; }
                        auto second()const{ return y_; }
                private:
                        rank_id x_;
                        rank_id y_;
                };

                template<class Tag>
                struct basic_rank_tuple{
                        explicit basic_rank_tuple(rank_id x, rank_id y):x_{x},y_{y}{}
                        auto first()const{ return x_; }
                        auto second()const{ return y_; }
                private:
                        rank_id x_;
                        rank_id y_;
                };

                using suited      = basic_rank_tuple<struct tag_suited>;
                using offsuit     = basic_rank_tuple<struct tag_offsuit>;
                using any_suit    = basic_rank_tuple<struct tag_any_suit>;

                using primitive_tl = boost::mpl::vector<hand, pocket_pair, offsuit, suited, any_suit>;

                using primitive_t = boost::variant<hand, pocket_pair, offsuit, suited, any_suit>;

                struct plus{
                        explicit plus(primitive_t const& prim):prim_{prim}{}
                        primitive_t const&  get()const{ return prim_; }
                private:
                        primitive_t prim_;
                };
                struct interval{
                        explicit interval(primitive_t const& first, primitive_t const& last):
                                first_{first}, last_{last}
                        {}
                        primitive_t const& first()const{ return first_; }
                        primitive_t const& last()const{ return last_; }
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
                                        *ostr_ << boost::format("pocker_pair{%s, %s}") % rank_decl::get(prim.first()) % rank_decl::get(prim.first());
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
                                        *ostr_ << rank_decl::get(prim.first()) << rank_decl::get(prim.first());
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
                }

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
                

                static auto _AA = pocket_pair(::ps::decl::_A);
                static auto _KK = pocket_pair(::ps::decl::_K);
                static auto _QQ = pocket_pair(::ps::decl::_Q);
                static auto _JJ = pocket_pair(::ps::decl::_J);
                static auto _TT = pocket_pair(::ps::decl::_T);
                static auto _99 = pocket_pair(::ps::decl::_9);
                static auto _88 = pocket_pair(::ps::decl::_8);
                static auto _77 = pocket_pair(::ps::decl::_7);
                static auto _66 = pocket_pair(::ps::decl::_6);
                static auto _55 = pocket_pair(::ps::decl::_5);
                static auto _44 = pocket_pair(::ps::decl::_4);
                static auto _33 = pocket_pair(::ps::decl::_3);
                static auto _22 = pocket_pair(::ps::decl::_2);

                static auto _AKo = suited(::ps::decl::_A, ::ps::decl::_K);
                static auto _AQo = suited(::ps::decl::_A, ::ps::decl::_Q);
                static auto _AJo = suited(::ps::decl::_A, ::ps::decl::_J);
                static auto _ATo = suited(::ps::decl::_A, ::ps::decl::_T);

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
                                        PRINT_SEQ((*iter)(std::string(iter,last))(stack_.size()));
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

                range parse(std::string const& str){
                        static detail::parser<decltype(str.begin())> p;
                        return p.parse(str.begin(), str.end());
                }

        }


}

#endif // PS_HOLDEM_FRONTEND_H
