#include "ps/base/frontend.h"

namespace ps{ namespace frontend{

        /*
         * This function maps the range structore to a subset,
         * so that seach sub range is exclusive, and 
         * plus and intervals are mapped to primtives
         */
        range expand(range const& self){
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
/*
        range         -> sub_range_seq
                
        sub_range_seq -> sub_range
                         sub_range sub_range_seq
                         sub_range,sub_range_seq
                         sub_range;sub_range_seq
        
        sub_range     -> hand
                      -> class_decl

        hand          -> rank suit rank suit

        rank          -> rRcChHdD

        suit          -> 23456789tTjJqQkKaA

        class_decl    -> class
                         interval
                         plus

        interval      -> class - class

        plus          -> class +

        class         -> rank rank 
                         rank rank suit_decl

        suit_decl     -> oOsS

        class_x       -> rank x
                         rank X

                 
 */

        namespace detail{
                template<class Iter_Type>
                struct parser{
                        range parse(Iter_Type start, Iter_Type end){
                                restart();

                                begin = start;
                                iter = begin;
                                last = end;

                                if( std::string{start,end} == "100%" )
                                        return percent(100);
                                        
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
                                                stack_.emplace_back( offsuit{rank_decl::parse(*iter), rank_decl::parse(*(iter+1))});
                                                break;
                                        case 's': case 'S':
                                                stack_.emplace_back( suited{rank_decl::parse(*iter), rank_decl::parse(*(iter+1))});
                                                break;
                                        }
                                        iter += m[0].length();
                                        return true;
                                }
                                if( std::regex_search( iter, last, m, rgx_XX) ){
                                        if( m[0].str()[0] == m[0].str()[1] ){
                                                stack_.emplace_back( pocket_pair{rank_decl::parse(*iter)});
                                        } else{
                                                stack_.emplace_back( any_suit{rank_decl::parse(*iter), rank_decl::parse(*(iter+1))});
                                        }
                                        iter += m[0].length();
                                        return true;
                                }
                                // AX+, Ax
                                if( std::regex_search( iter, last, m, rgx_X_) ){
                                        stack_.emplace_back( any_suit{rank_decl::parse(*iter), rank_decl::parse('2')});;
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

} } // namespace frontend  , ps 
