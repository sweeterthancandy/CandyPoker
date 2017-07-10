#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <codecvt>
#include <experimental/filesystem>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/regex_actions.hpp>

#include "ps/detail/print.h"

#include "ps/parser/subparser.h"
#include "ps/parser/parser_context.h"
        
namespace fs = std::experimental::filesystem;


enum Street{
        Street_Preflop,
        Street_Flop,
        Street_Turn,
        Street_River
};
        
enum HandType{
        HandType_Cash,
        HandType_Tournament
};

struct decl_type{
        enum DeclType{
                Decl_Seat,
                Decl_Button,
                Decl_NumPlayers,
                Decl_Event
        };


        decl_type(DeclType type):type_{type}{}
        virtual ~decl_type()=default;

        auto type()const{ return type_; }
private:
        DeclType type_;
};

struct event_type : decl_type{
        enum EventType{
                Event_PostAnte,
                Event_PostSB,
                Event_PostBB,
                Event_Fold,
                Event_Call,
                Event_Raise,
                Event_FlopDeal,
                Event_TurnDeal,
                Event_RiverDeal
        };

        event_type(EventType event):decl_type{Decl_Event}, event_{event}{}

        auto event()const{ return event_; }
private:
        EventType event_;
};



/*

   Need to be able to detect the kind of hands they are, ie find
   all finds which where are preflop allin/fold situtation

*/

#if 0
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
#endif

#if 0



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
        std::shared_ptr< parser_decl > make
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

std::shared_ptr< parser_decl > make_ps_parser(){
        parser_builder b;

        auto header{

}




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
#endif

#if 0
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
#endif


namespace ps{
namespace parser{
using namespace xpr;


}
}

struct pokerstars_parser{
         void parse(std::vector<std::string> const& lines){
                 namespace xpr = boost::xpressive;
                 ps::parser::stream_parser_context ctx{std::cout};

                 xpr::sregex rgx, tmp;

                 auto header{ps::parser::make("header", ctx)};
                 auto button_decl{ps::parser::make("button_decl",ctx)};
                 auto call{ps::parser::make("call", ctx)};
                 auto fold{ps::parser::make("fold", ctx)};
                 auto bet{ps::parser::make("bet", ctx)};
                 auto decl_deal{ps::parser::make("decl_deal", ctx)};
                 auto decl_section{ps::parser::make("decl_section", ctx)};
                 auto post{ps::parser::make("post", ctx)};
                 auto seat_decl{ps::parser::make("seat_decl", ctx)};
                 auto raise{ps::parser::make("raise", ctx)};
                 auto mucks{ps::parser::make("mucks", ctx)};
                 auto uncalled_bet_returned{ps::parser::make("uncalled_bet_returned", ctx)};
                 auto check{ps::parser::make("check", ctx)};
                 auto collected_from_pot{ps::parser::make("collected_from_pot", ctx)};
                 auto doesnt_show_hand{ps::parser::make("doesnt_show_hand", ctx)};
                 auto decl_total_pot{ps::parser::make("decl_total_pot", ctx)};
                 auto decl_flop{ps::parser::make("decl_flop", ctx)};
                 auto shows{ps::parser::make("shows", ctx)};
                 auto decl_seat_summary{ps::parser::make("decl_seat_summary", ctx)};
                 rgx =
                        *header                 |
                        *button_decl            |
                        *call                   |
                        *fold                   |
                        *mucks                  |
                        *bet                    |
                        *decl_deal              |
                        *decl_section           |
                        *post                   |
                        *seat_decl              |
                        *raise                  |
                        *check                  |
                        *uncalled_bet_returned  |
                        *doesnt_show_hand       |
                        *decl_total_pot         |
                        *decl_flop              |
                        *shows                  |
                        *decl_seat_summary      |
                        *collected_from_pot
                 ;



                 for(auto const& l : lines){
                         xpr::sregex ws{ xpr::sregex::compile(R"(\s+$)")};
                         auto stripped( xpr::regex_replace(l, ws, "") );
                         if( ! xpr::regex_search( stripped, rgx ) ){
                                 PRINT(l);
                                 return;
                         }
                 }
                
         }
private:
};

struct parser{
        parser(){
        }

        void section_parser_(std::vector<std::string>&& lines){
                #if 0
                auto subparser{ parser_global_factory::get()->detect(lines) };
                auto hand{ subparser->parse(line) };
                hand.display();
                #endif
                pokerstars_parser ps;
                ps.parse(lines);
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
                //PRINT(f);
                std::ifstream ifstr(f, std::ifstream::binary);
                if( ! ifstr.is_open()){
                        std::cerr << "unable to open " << f << "\n";
                        continue;
                }
                p.parse(ifstr);
        }
        

}
