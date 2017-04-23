#ifndef PS_DETAIL_PRINT_H
#define PS_DETAIL_PRINT_H

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <ostream>

#include <boost/preprocessor.hpp>

#define PRINT_SEQ_detail(r, d, i, e) do{ std::cout << ( i ? ", " : "" ) << BOOST_PP_STRINGIZE(e) << " = " << (e); }while(0);
#define PRINT_SEQ(SEQ) do{ BOOST_PP_SEQ_FOR_EACH_I( PRINT_SEQ_detail, ~, SEQ) std::cout << "\n"; }while(0)
#define PRINT(X) PRINT_SEQ((X))
#define PRINT_TEST( EXPR ) do{ bool ret(EXPR); std::cout << std::setw(50) << std::left << #EXPR << std::setw(0) << ( ret ? " OK" : " FAILED" ) << "\n"; }while(0);

namespace ps{
        namespace detail{
                template<class Con>
                std::ostream& to_string(std::ostream& ostr, Con const& con){
                        if( con.empty())
                                return ostr << "{}";
                        ostr << "{";

                        if( std::next(con.begin()) != con.end()){
                                std::copy( con.begin(), std::prev(con.end()), std::ostream_iterator<
                                           std::decay_t<decltype(*con.begin())> >(ostr, ", "));
                        }
                        ostr << con.back() << "}";
                        return ostr;
                }
        }
}

#endif // PS_DETAIL_PRINT_H
