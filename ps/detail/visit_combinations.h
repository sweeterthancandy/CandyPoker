#ifndef PS_DETAIL_VISIT_COMBINATIONS_H
#define PS_DETAIL_VISIT_COMBINATIONS_H


namespace ps{
        namespace detail{
                template<int N, class V, class F, class... Args>
                        std::enable_if_t<N==0> visit_combinations(V v, F f, long upper, Args&&... args){
                                v(std::forward<Args>(args)...);
                        }
                template<int N, class V, class F, class... Args>
                        std::enable_if_t<N!=0> visit_combinations(V v, F f, long upper, Args&&... args){
                                for(long iter{upper+1};iter!=0;){
                                        --iter;
                                        if( ! f(iter) )
                                                continue;
                                        visit_combinations<N-1>(v, f, iter-1,std::forward<Args>(args)..., iter);
                                }
                        }
        } // namespace detail
} // namespace ps

#endif // PS_DETAIL_VISIT_COMBINATIONS_H
