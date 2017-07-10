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

namespace xpr = boost::xpressive;

struct parser_context{
        int make(){ return 1; }
        void begin_hand(){}
};

using namespace xpr;



xpr::sregex& make_header(parser_context& ctx){
        struct on_header_impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context& ctx, std::string const& line) const
                {
                        //std::cout << "header : " << line << "\n";
                }
        };
        static xpr::function<on_header_impl>::type const on_header = {{}};
        static sregex r{xpr::sregex::compile(R"(^PokerStars Hand #[[:digit:]]+)")
                [ on_header(std::ref(ctx), _) ]
        };
        return r;
}

xpr::sregex& make_button_decl(parser_context& ctx){
        struct on_button_decl_impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx, std::string const& line) const
                {
                        //std::cout << "btn decl : " << line << "\n";
                }
        };

        static xpr::function<on_button_decl_impl>::type const on_button_decl = {{}};
                // Table '1863148423 21' 9-max Seat #7 is the button
        static sregex r{
                xpr::sregex::compile(R"(Table '\d+ \d+' \d-max Seat #\d+ is the button)")
                [ on_button_decl(std::ref(ctx), _) ]
        };
        return r;
}

xpr::sregex& make_seat_decl(parser_context& ctx){
        struct on_seat_decl_impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                int seat,
                                std::string const& name,
                                std::string const& stack) const
                {
                        PRINT_SEQ((seat)(name)(stack));
                        //std::cout << "btn decl : " << line << "\n";
                }
        };

        static xpr::function<on_seat_decl_impl>::type const on_seat_decl = {{}};
                // Seat 1: bileo27 (25595 in chips)
        static sregex r{
                (xpr::bos >> "Seat " >> (s1=+_d) >> ": " >> (s2=+_) >> "(" >> (s3=+_d) >> " in chips)" >> *_s >> xpr::eos)
                [ on_seat_decl(std::ref(ctx), _, as<int>(s1), s2, s3) ]
        };
        return r;
}

xpr::sregex& make_post(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& stack,
                                std::string const& token) const
                {
                        PRINT_SEQ((name)(stack)(token));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        static sregex r{
                (xpr::bos >> (s1=+_) >> ": posts " >> (s3=xpr::sregex::compile("(the ante|small blind|big blind)")) >> *_s >> (s2=+~_s))
                [ f(std::ref(ctx), _, s1, s2, s3) ]
        };
        return r;
}

xpr::sregex& make_decl_section(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& token) const
                {
                        PRINT_SEQ((token));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        static sregex r{
                (xpr::bos >> "*** " >> (s1=+_) >> " ***")
                [ f(std::ref(ctx), _, s1) ]
        };
        return r;
}

xpr::sregex& make_decl_deal(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& hand) const
                {
                        PRINT_SEQ((name)(hand));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        static sregex r{
                (xpr::bos >> "Dealt to " >> (s1=+_) >> *_s >> "[" >> (s2=+_) >> "]" >> *_s >> xpr::eos)
                [ f(std::ref(ctx), _, s1, s2) ]
        };
        return r;
}
xpr::sregex& make_fold(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name
                                ) const
                {
                        PRINT_SEQ((name));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        static sregex r{
                (xpr::bos >> (s1=+_) >> ": folds" >> xpr::eos )
                [ f(std::ref(ctx), _, s1) ]
        };
        return r;
}
xpr::sregex& make_call(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& stack
                                ) const
                {
                        PRINT_SEQ((name)(stack));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        static sregex r{
                (xpr::bos >> (s1=+_) >> ": calls " >> (s2=+~_s) >> optional(" and is all-in"))
                [ f(std::ref(ctx), _, s1, s2) ]
        };
        return r;
}

struct pokerstars_parser{
         void parse(std::vector<std::string> const& lines){
                 parser_context ctx;

                 using xpr::_d;
                 using namespace xpr;

                 xpr::sregex rgx, tmp;

                 #if 0
                 rgx  = make_button_decl(ctx);
                 rgx |= make_header(ctx);
                 //rgx |= make_seat_decl(ctx);
                 #endif
                 rgx  =
                        make_button_decl(ctx) |
                        make_header(ctx)      |
                        make_seat_decl(ctx)   |
                        make_post(ctx)        |
                        make_decl_section(ctx)|
                        make_decl_deal(ctx)   |
                        make_fold(ctx)        |
                        make_call(ctx)      

                ;

                 pctx_.begin_hand();

                 for(auto const& l : lines){
                         xpr::sregex ws{ xpr::sregex::compile(R"(\s+$)")};
                         auto stripped( xpr::regex_replace(l, ws, "") );
                         if( ! xpr::regex_search( stripped, rgx ) ){
                                 PRINT(l);
                                 return;
                         }
                 }
                
                 auto result{pctx_.make()};
         }
private:
         parser_context pctx_;
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
