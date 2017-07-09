#ifndef PS_DETAIL_VISIT_SEQUENCE_H
#define PS_DETAIL_VISIT_SEQUENCE_H

#include <utility>

namespace ps{
namespace detail{

using iseq_2_9 = std::integer_sequence<size_t,2,3,4,5,6,7,8,9>;

template<class F, class T, T... values >
void visit_iseq_detail( std::integer_sequence<T, values...>, F f){
        int dummy[] = { 0, (f(boost::mpl::size_t<values>{}, values...),0)...};
}
template<class Seq, class F>
void visit_iseq( F f ){
        visit_iseq_detail(Seq{}, f);
}


template<class F>
void visit_iseq_2_9(F&& f){
        visit_iseq<iseq_2_9>(f);
}



} // defail
} // ps

#if 0
        visit_iseq_2_9( [](auto n){
                constexpr const size_t N = decltype(n)::value;
                PRINT(n);
        });


#endif



#endif // PS_DETAIL_VISIT_SEQUENCE_H
