#ifndef PS_DETAIL_PRINT_H
#define PS_DETAIL_PRINT_H

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <ostream>
#include <iostream>

#include <boost/preprocessor.hpp>
#include <boost/lexical_cast.hpp>


#define STREAM_SEQ_detail(r, d, i, e) do{ d << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define STREAM_SEQ(STR, SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( STREAM_SEQ_detail, STR, SEQ) STR << "\n"; }while(0)
#define STREAM_PRINT(STR, X) STREAM_SEQ(STR, (X))

#define PRINT_SEQ(SEQ) STREAM_SEQ(std::cout, SEQ)
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
