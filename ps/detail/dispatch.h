#ifndef PS_DETAIL_DISPATCH_H
#define PS_DETAIL_DISPATCH_H

namespace ps{
namespace detail{
        template<class MatrixType, class ArrayType>
        void dispatch_ranked_vector_mat(MatrixType& result, ArrayType const& ranked, size_t n)noexcept{
                auto lowest = ranked[0] ;
                size_t count{1};
                for(size_t i=1;i<n;++i){
                        if( ranked[i] == lowest ){
                                ++count;
                        } else if( ranked[i] < lowest ){
                                lowest = ranked[i]; 
                                count = 1;
                        }
                }
                for(size_t i=0;i!=n;++i){
                        if( ranked[i] == lowest ){
                                ++result(count-1, i);
                        }
                }
        }
} // detail
} // ps

#endif // PS_DETAIL_DISPATCH_H
