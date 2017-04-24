#ifndef PS_TRANSFORMS_TREE_PRINTER_H
#define PS_TRANSFORMS_TREE_PRINTER_H

#include <boost/mpl/char.hpp>
#include <boost/lexical_cast.hpp>

namespace ps{
namespace transforms{

        struct tree_printer_detail{

                void non_terminal(const std::string& line){
                        terminal(line);
                        stack_.emplace_back(line.size());
                }
                void terminal(const std::string& line){
                        if( stack_.size() ){
                                std::for_each( stack_.begin(), std::prev(stack_.end()),
                                        [this](format_data& fd){
                                                fd.print(std::cout);
                                });
                                stack_.back().print_alt(std::cout);
                        }
                        std::cout << line << "\n";
                }
                void begin_children_size(size_t n){
                        stack_.back().finalize(n);
                }
                void end_children(){
                        stack_.pop_back();
                }
                void next_child(){
                        stack_.back().next();
                }
        private:

                struct angle_c_traits{
                        using hor_c = boost::mpl::char_<'_'>;
                        using vert_c = boost::mpl::char_<'|'>;
                        using br_c = boost::mpl::char_<'\\'>;
                };
                struct straight_c_traits{
                        using hor_c = boost::mpl::char_<'_'>;
                        using vert_c = boost::mpl::char_<'|'>;
                        using br_c = boost::mpl::char_<'|'>;
                };
                
                using c_traits = angle_c_traits;

                struct format_data{
                        std::string make_(char c, char a){
                                std::string tmp;
                                tmp += std::string((indent_-1)/2,' ');
                                tmp += c;
                                tmp += std::string(indent_-tmp.size(),a);
                                return tmp;
                        }
                        format_data(size_t indent):indent_(indent){
                                auto sz = indent_;
                        }
                        void finalize(size_t num_children){
                                num_children_ = num_children;

                        }
                        void print_alt(std::ostream& ostr){
                                if( indent_ )
                                        ostr << make_(c_traits::br_c(), c_traits::hor_c());
                        }
                        void print(std::ostream& ostr){
                                if( indent_ ){
                                        if( idx_ != num_children_ -1 ){
                                                ostr << make_(c_traits::vert_c(), ' ');
                                        } else{
                                                ostr << std::string(indent_,' ');
                                        }
                                }
                        }
                        void next(){
                                ++idx_;
                                assert( idx_ <= num_children_ );
                        }
                private:
                        size_t idx_{0}; // child index
                        size_t num_children_; // number of children
                        const size_t indent_; // lengh of indent
                };
                std::list<format_data> stack_;
        };

        struct tree_printer : symbolic_transform{
                tree_printer():symbolic_transform{"tree_printer"}{}
                bool apply(symbolic_computation::handle& ptr)override{
                        std::stringstream line;

                        switch(ptr->get_kind()){
                        case symbolic_computation::Kind_Symbolic_Primitive: {
                                auto sp{ reinterpret_cast<symbolic_primitive*>(ptr.get()) };
                                for(size_t i{0};i!=sp->get_hands().size();++i){
                                        if( i != 0 ) line << " vs ";
                                        line << sp->get_hands()[i];
                                }
                                impl_.terminal(line.str());
                        }
                                return false;
                        case symbolic_computation::Kind_Symbolic_Range: {
                                auto sr{ reinterpret_cast<symbolic_range*>(ptr.get()) };
                                for(size_t i{0};i!=sr->get_players().size();++i){
                                        if( i != 0 ) line << " vs ";
                                        line << sr->get_players()[i];
                                }
                                impl_.non_terminal(line.str());
                        }
                                return false;
                        case symbolic_computation::Kind_Symbolic_Primitive_Range: {
                                auto spr{ reinterpret_cast<symbolic_primitive_range*>(ptr.get()) };
                                for(size_t i{0};i!=spr->get_players().size();++i){
                                        if( i != 0 ) line << " vs ";
                                        line << spr->get_players()[i];
                                }
                                impl_.non_terminal(line.str());
                        }
                                return false;
                        case symbolic_computation::Kind_Symbolic_Player_Perm: {
                                auto spp{ reinterpret_cast<symbolic_player_perm*>(ptr.get()) };
                                line << "p{";
                                for(size_t i{0};i!=spp->get_player_perm().size();++i){
                                        if( i != 0 ) line << " ";
                                        line << spp->get_player_perm()[i];
                                }
                                line << "}";
                                impl_.non_terminal(line.str());
                        }
                                return false;
                        case symbolic_computation::Kind_Symbolic_Suit_Perm: {
                                auto ssp{ reinterpret_cast<symbolic_suit_perm*>(ptr.get()) };
                                line << "s{";
                                for(size_t i{0};i!=ssp->get_suit_perm().size();++i){
                                        if( i != 0 ) line << " ";
                                        line << ssp->get_suit_perm()[i];
                                }
                                line << "}";
                                impl_.non_terminal(line.str());
                                return false;
                        }
                        }
                                

                        return false;
                }
                void begin_children(size_t idx)override{
                        impl_.begin_children_size(idx);
                }
                void end_children()override{
                        impl_.end_children();
                }
                void next_child()override{
                        impl_.next_child();
                }
        private:
                tree_printer_detail impl_;
        };


} // transform
} // ps

#endif // PS_TRANSFORMS_TREE_PRINTER_H
