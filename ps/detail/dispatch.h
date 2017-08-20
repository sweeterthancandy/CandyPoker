#ifndef PS_DETAIL_DISPATCH_H
#define PS_DETAIL_DISPATCH_H

namespace ps{
namespace detail{
        struct dispatch_ranked_vector{
                void operator()(equity_breakdown_matrix& result, 
                                std::vector<ranking_t> const& ranked)const
                {
                        auto lowest = ranked[0] ;
                        size_t count{1};
                        for(size_t i=1;i<ranked.size();++i){
                                if( ranked[i] == lowest ){
                                        ++count;
                                } else if( ranked[i] < lowest ){
                                        lowest = ranked[i]; 
                                        count = 1;
                                }
                        }
                        for(size_t i=0;i!=ranked.size();++i){
                                if( ranked[i] == lowest ){
                                        ++result.data_access(i,count-1);
                                }
                        }
                        ++result.sigma();
                }
        };
} // detail
} // ps

#endif // PS_DETAIL_DISPATCH_H
