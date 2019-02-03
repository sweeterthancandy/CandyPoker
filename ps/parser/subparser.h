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
#ifndef PS_PARSER_SUBPARSER_H
#define PS_PARSER_SUBPARSER_H

#include <map>
#include <string>
#include <functional>

#include <boost/exception/all.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/preprocessor/cat.hpp>

#include "ps/detail/print.h"

namespace ps{
namespace parser{

struct parser_context;

namespace xpr = boost::xpressive;

struct subparser_factory{
        using rgx_ptr_t = xpr::sregex*;
        using tefun_t = std::function<std::shared_ptr<xpr::sregex>(parser_context&)>;

        void register_me(std::string name, tefun_t f){
                world_[name] = f;
        }
        std::shared_ptr<xpr::sregex> make(std::string const& name, parser_context& ctx){
                auto iter =  world_.find(name) ;
                if( iter == world_.end() )
                        BOOST_THROW_EXCEPTION(std::domain_error("no subparser " + name));
                return iter->second(ctx);
        }

        static subparser_factory* get(){
                static subparser_factory* mem = 0;
                if( mem == 0 )
                        mem = new subparser_factory;
                return mem;
        }
private:
        std::map<std::string, tefun_t> world_;
};

// how about come sugar
inline
auto make(std::string name, parser_context& ctx){
        return ps::parser::subparser_factory::get()
                ->make(name, ctx);
}

#define SUBPARSER_FACTORY_REGISTER(name, f)                             \
        int BOOST_PP_CAT(autoreg__,__LINE__)  =                         \
                (::ps::parser::subparser_factory::get()->register_me(   \
                        #name,                                          \
                        [](parser_context& ctx){                        \
                                return f(ctx);                          \
                        }), 0);                                        


} // parser
} // ps


#endif // PS_PARSER_SUBPARSER_H
