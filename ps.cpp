#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <codecvt>
#include <experimental/filesystem>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "ps/detail/print.h"
        
namespace fs = std::experimental::filesystem;

/*

   Need to be able to detect the kind of hands they are, ie find
   all finds which where are preflop allin/fold situtation

*/

struct player_decl{
        void decl_name(std::string const& name){ return name_; }
        void decl_stack(double stack){ return stack_; }
private:
        boost::optional<std::string> name_;
        double stack_;
};

struct game_decl{
        void decl_sb(double sb){ sb_ = sb; }
        void decl_bb(double bb){ bb_ = bb; }
        void decl_player(player_decl const& p){ players_.push_back(p); }
private:
        boost::optional<double> sb_;
        boost::optional<double> bb_;
        std::vector< player_decl > players_;
};

using player_id = int;

/*
 *  From past experince I want this
 */
enum SubParserCtrl{
        SubParserCtrl_Parsed,
        SubParserCtrl_Failed
};
struct subparser
{
        using line_iterator = std::vector<std::string>::const_iterator;

        virtual ~subparser()=default;

        virtual SubParserCtrl execute( parser_context& ctx)=0;
};

struct subparser_or{
        SubParserCtrl execute( parser_context& ctx)override{
                for(auto child : children_){
                        switch(child->execute( ctx )){
                        case SubParserCtrl_Parsed:
                                return SubParserCtrl_Parsed;
                        default:
                                break;
                        }
                }
                return SubParserCtrl_Failed;
        }
private:
        std::vector<subparser*> children_;
};



namespace pokerstars{
        
        // by domain
        namespace header{

                struct tournament{
                        SubParserCtrl execute( parser_context& ctx, factory& fac)override{
                                std::smatch m;
                                if( std::regex_searh(ctx.current_line(), m, rgx_ ) ){
                                        fac.add( 
                                }
                        }
                private:
                        std::regex rgx_{R"(^PokerStars Hand #(\d+): Tournament #(\d+),)"};
                        //std::regex rgx_{R"(PokerStars Hand #(\d+): Tournament #(\d+), $14.10+$0.90 USD Hold'em No Limit - Level I (10/20) - 2017/07/02 14:36:52 WET [2017/07/02 9:36:52 ET])"};
                };
        }
}



struct parser_global_factory{

        template<class T>
        void register_subparser(std::string const& name,
                                std::string const& domain){
                auto maker = [](){ return std::make_shared<T>(); };
                maker_.insert(std::make_pair(name, maker));
                dmaker_[domain] = maker;
        }

        parser_global_factory* get(){
                static parser_global_factory* mem = 0;
                if( ! mem )
                        mem = new parser_global_factory;
                return mem;
        }
private:
        std::map<std::string, std::function<std::shared_ptr<subparser>()> > maker_;
        std::map<std::string, std::vector<std::function<std::shared_ptr<subparser>()> > > maker_;
};

struct action_post_sb{
        action_post_sb(player_id player):
                player_(player)
        {}
        void execute( factory& fac){
                fac.post_sb();
        }
private:
        player_id id_;
};


struct parser_item{
        std::regex rgx;
};
struct parser_namespace{
        std::vector<parser_item> item_;
};
struct parser_factory{
};
struct parser_context{
};

struct parser{
        parser(){
                factory fac;
                auto root   = new subparser_or{};
                
                auto header = new subparser_or{};
                header->add( new ps_tourn_header{} );
                
                auto info   = new subparser_header;
                info->dd(new ps_tourn_



        }

        void section_parser_(std::vector<std::string>&& lines){
                
                for( auto const& l : lines)
                        std::cout << l << "\n";
                std::cout << "END_SECTION\n";
        }
        void parse(std::istream& istr){
                std::vector<std::string> lines;

                auto produce = [&](){
                        section_parser_(std::move(lines));
                        lines.clear();
                };


                for(;;){
                        std::string raw_line;
                        std::getline(istr, raw_line);
                        if( istr.eof() && raw_line.empty())
                                break;

                        if( istr.eof())
                                break;

                        if( std::regex_match(raw_line, whitespace_rgx_ ) ){
                                if( lines.size())
                                       produce();
                        } else {
                                lines.push_back(std::move(raw_line));
                        }
                }

                if( lines.size() ){
                        produce();
                }
        }
private:
        std::regex whitespace_rgx_{R"(\s+)"};
        parser_context pc_proto_;
};


int main(){


        std::regex HH_rgx("^HH");
        std::stack<fs::path> stack;
        std::vector<std::string> to_process;
        stack.emplace("hh/2017");
        for( ;stack.size(); ){
                auto item{ stack.top() };
                stack.pop();
                for(fs::directory_iterator di(item),de;di!=de;++di){
                        switch(di->status().type()){
                        case fs::file_type::regular:
                                if( std::regex_search( di->path().filename().string(), HH_rgx ) ){
                                        to_process.push_back( di->path().string() );
                                }
                                break;
                        case fs::file_type::directory:
                                stack.push(di->path());
                                break;
                        }
                }
        }
        parser p;
        int i=0;
        for( auto const& f : to_process ){
                PRINT(f);
                std::ifstream ifstr(f, std::ifstream::binary);
                if( ! ifstr.is_open()){
                        std::cerr << "unable to open " << f << "\n";
                        continue;
                }
                p.parse(ifstr);
        }
        

}
