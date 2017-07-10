#include "ps/parser/parser.h"

#include <boost/xpressive/regex_actions.hpp>

#include "ps/detail/print.h"

namespace ps{
namespace parser{

using namespace xpr;

auto make_header(parser_context& ctx){
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
        return
                std::make_shared<xpr::sregex>(
                        xpr::sregex::compile(R"(^PokerStars Hand #[[:digit:]]+)")
                        [ on_header(std::ref(ctx), _) ]
                );
}
SUBPARSER_FACTORY_REGISTER(header, make_header)

auto make_button_decl(parser_context& ctx){
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
        return std::make_shared<sregex>(
                xpr::sregex::compile(R"(Table '\d+ \d+' \d-max Seat #\d+ is the button)")
                [ on_button_decl(std::ref(ctx), _) ]
        );
}

SUBPARSER_FACTORY_REGISTER(button_decl, make_button_decl)

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

} // paser
} // ps
