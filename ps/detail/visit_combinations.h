#ifndef PS_DETAIL_VISIT_COMBINATIONS_H
#define PS_DETAIL_VISIT_COMBINATIONS_H


namespace ps{
        namespace detail{

                static auto true_ = [](auto...){return true; };

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
                template<int N, class V, class... Args>
		auto visit_combinations(V v, long upper, Args&&... args){
			return visit_combinations<N>(v,true_, upper, std::forward<Args>(args)...);
		}

                template<int N, class V, class F, class Upper, class... Args>
		std::enable_if_t<N==0> visit_exclusive_combinations(V v, F f, Upper upper, Args&&... args){
			v(std::forward<Args>(args)...);
		}
                template<int N, class V, class F, class Upper, class... Args>
		std::enable_if_t<N!=0> visit_exclusive_combinations(V v, F f, Upper upper, Args&&... args){
			for(auto iter{upper[N-1]+1};iter!=0;){
				--iter;
				if( ! f(iter) )
					continue;
				visit_exclusive_combinations<N-1>(v, f, upper, iter, std::forward<Args>(args)...);
			}
		}
                


		
        } // namespace detail
} // namespace ps

#endif // PS_DETAIL_VISIT_COMBINATIONS_H
