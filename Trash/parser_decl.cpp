#include "ps/parser/parser.h"

#include <boost/xpressive/regex_actions.hpp>

#include "ps/detail/print.h"

namespace ps{
namespace parser{
namespace {

using namespace xpr;

auto make_header(parser_context& ctx){
        struct on_header_impl
        {
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

auto make_seat_decl(parser_context& ctx){
        struct on_seat_decl_impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                int seat,
                                std::string const& name,
                                std::string const& stack) const
                {
                        //PRINT_SEQ((seat)(name)(stack));
                        //std::cout << "btn decl : " << line << "\n";
                }
        };

        static xpr::function<on_seat_decl_impl>::type const on_seat_decl = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> "Seat " >> (s1=+_d) >> ": " >> (s2=+_) >> "(" >> (s3=+_d) >> " in chips)")
                [ on_seat_decl(std::ref(ctx), _, as<int>(s1), s2, s3) ]
        );
}

SUBPARSER_FACTORY_REGISTER(seat_decl, make_seat_decl)

auto make_post(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& stack,
                                std::string const& token) const
                {
                        //PRINT_SEQ((name)(stack)(token));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": posts " >> (s3=xpr::sregex::compile("(the ante|small blind|big blind)")) >> *_s >> (s2=+~_s))
                [ f(std::ref(ctx), _, s1, s2, s3) ]
        );
}

SUBPARSER_FACTORY_REGISTER(post, make_post)

auto make_decl_section(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& token) const
                {
                        //PRINT_SEQ((token));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> "*** " >> (s1=+_) >> " ***")
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_section, make_decl_section)

auto make_decl_deal(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& hand) const
                {
                        //PRINT_SEQ((name)(hand));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                
        return std::make_shared<sregex>(
                (xpr::bos >> "Dealt to " >> (s1=+_) >> *_s >> "[" >> (s2=+_) >> "]" >> *_s >> xpr::eos)
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_deal, make_decl_deal)

auto make_fold(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name
                                ) const
                {
                        //PRINT_SEQ((name));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": folds" >> xpr::eos )
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(fold, make_fold)

auto make_call(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& stack
                                ) const
                {
                        //PRINT_SEQ((name)(stack));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": calls " >> (s2=+~_s) >> optional(" and is all-in"))
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(call, make_call)

auto make_raise(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& name,
                                std::string const& from,
                                std::string const& to
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": raises " >> (s2=+~_s) >> " to " >> (s3=+~_s) >> optional(" and is all-in"))
                [ f(std::ref(ctx), _, s1, s2, s3) ]
        );
}

SUBPARSER_FACTORY_REGISTER(raise, make_raise)

auto make_uncalled_bet_returned(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& bet,
                                std::string const& to
                                ) const
                {
                        //PRINT_SEQ((line)(bet)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> "Uncalled bet (" >> (s1=+_) >> ") returned to " >> (s2=+~_s))
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(uncalled_bet_returned, make_uncalled_bet_returned)

auto make_check(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& player
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": checks")
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(check, make_check)

auto make_collected_from_pot(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who,
                                std::string const& value
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> " collected " >> (s2=+~_s) >> " from pot")
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(collected_from_pot, make_collected_from_pot)

auto make_bet(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who,
                                std::string const& value
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": bets " >> (s2=+~_s))
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(bet, make_bet)

auto make_doesnt_show_hand(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": doesn't show hand")
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(doesnt_show_hand, make_doesnt_show_hand)

auto make_decl_total_pot(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& value,
                                std::string const& rake
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> "Total pot " >> (s1=+~_s) >> " | Rake " >> (s2=+~_s))
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_total_pot, make_decl_total_pot)

auto make_decl_flop(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& flop
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> "Board [" >> (s1=+_) >> "]" >> xpr::eos)
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_flop, make_decl_flop)


auto make_decl_seat_summary(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& seat,
                                std::string const& who,
                                std::string const& show
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                        //PRINT(show);
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos 
                 >> "Seat " >> (s1=_d) >> ": " >> (s2=+_) >> " " 
                 #if 1
                 >> optional( ( as_xpr("(big blind)")   |
                                       "(small blind)" ) >> " " )
                 >> optional( as_xpr("(button) ") >> " " )
                 #endif
                 >> ( ( as_xpr("collected (")  >> +_d >> ")"  ) |
                      ( as_xpr("showed [") >> +_ >> "]"   ) |
                      ( as_xpr("mucked [") >> +_ >> "]"   ) |
                        //as_xpr("shows")                         |
                        as_xpr("folded before Flop")            |
                        as_xpr("folded on the Flop")            |
                        as_xpr("folded on the Turn")            |
                        as_xpr("folded on the River")          
                    )
                )
                [ f(std::ref(ctx), _, s1, s2, s3) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_seat_summary, make_decl_seat_summary)

auto make_shows(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who,
                                std::string const& hand
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": shows [" >> (s2=+_) >> "]")
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(shows, make_shows)

auto make_mucks(parser_context& ctx){
        struct impl
        {
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> ": mucks hand")
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(mucks, make_mucks)

auto make_decl_finish_tournament(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who,
                                std::string const& place
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> " finished the tournament in " >> (s2=+~_s) >> " place")
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_finish_tournament, make_decl_finish_tournament)

auto make_decl_win_tournament(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who,
                                std::string const& prize
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> " wins the tournament and receives " >> (s2=+~_s) >> " - congratulations!")
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_win_tournament, make_decl_win_tournament)

auto make_decl_connection(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who,
                                std::string const& what
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> " is " >> (s2=( as_xpr("connected") |
                                                               "disconnected") ) )
                [ f(std::ref(ctx), _, s1, s2) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_connection, make_decl_connection)

auto make_decl_returned(parser_context& ctx){
        struct impl
        {
                // Result type, needed for tr1::result_of
                typedef void result_type;

                void operator()(parser_context const& ctx,
                                std::string const& line,
                                std::string const& who
                                ) const
                {
                        //PRINT_SEQ((line)(name)(from)(to));
                }
        };

        static xpr::function<impl>::type const f = {{}};
                // Seat 1: bileo27 (25595 in chips)
        return std::make_shared<sregex>(
                (xpr::bos >> (s1=+_) >> " has returned")
                [ f(std::ref(ctx), _, s1) ]
        );
}

SUBPARSER_FACTORY_REGISTER(decl_returned, make_decl_returned)
} // anon
} // paser
} // ps
