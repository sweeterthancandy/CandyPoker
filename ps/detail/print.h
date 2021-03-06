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
#ifndef PS_DETAIL_PRINT_H
#define PS_DETAIL_PRINT_H

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <ostream>
#include <iostream>

#include <boost/preprocessor.hpp>
#include <boost/lexical_cast.hpp>

#define PRINT_SEQ_detail(r, d, i, e) do{ std::cout << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define PRINT_SEQ(SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( PRINT_SEQ_detail, ~, SEQ) std::cout << "\n"; }while(0)
#define PRINT(X) PRINT_SEQ((X))
#define PRINT_TEST( EXPR ) do{ bool ret(EXPR); std::cout << std::setw(50) << std::left << #EXPR << std::setw(0) << ( ret ? " OK" : " FAILED" ) << "\n"; }while(0);

namespace ps{
        namespace detail{
                struct pretty_set_traits{
                        const char* start()const{
                                return "{";
                        }
                        const char* end()const{
                                return "}";
                        }
                        const char* sep()const{
                                return ", ";
                        }
                };
                struct pretty_array_traits{
                        const char* start()const{
                                return "[";
                        }
                        const char* end()const{
                                return "]";
                        }
                        const char* sep()const{
                                return ", ";
                        }
                };
                struct pretty_vs_traits{
                        const char* start()const{
                                return "";
                        }
                        const char* end()const{
                                return "";
                        }
                        const char* sep()const{
                                return " vs ";
                        }
                };
                struct lexical_caster{
                        template<class T>
                        std::string operator()(T const& val)const{
                                return boost::lexical_cast<std::string>(val);
                        }
                };
                template<class Con, class Caster = lexical_caster, class Traits = pretty_set_traits>
                std::string to_string(Con const& con,
                                      Caster const& im = Caster{},
                                      Traits const& t = Traits{}){
                        std::stringstream sstr;

                        auto iter =  std::begin(con) ;
                        auto end =  std::end(con) ;

                        if( iter == end ){
                                sstr << t.start() << t.end();
                                return sstr.str();
                        }
                        auto prev =  std::prev(end) ;

                        sstr << t.start();

                        for( ; iter != prev; ++iter){
                                sstr << im(*iter) << t.sep();
                        }

                        sstr << im(*iter);
                                
                        sstr << t.end();
                        return sstr.str();
                }

        }
}

#endif // PS_DETAIL_PRINT_H
