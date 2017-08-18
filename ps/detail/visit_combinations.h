#ifndef PS_DETAIL_VISIT_COMBINATIONS_H
#define PS_DETAIL_VISIT_COMBINATIONS_H


namespace ps{
namespace detail{

        template<int I>
        struct precedence_device : precedence_device<I-1>{};

        template<>
        struct precedence_device<0>{};

        using default_ = precedence_device<0>;
        
        template<class T, class Int_Type>
        auto deref_or_get_dispatch_(default_, T const& val, Int_Type index)
        {
                return val;
        }
        
        template<class T, class Int_Type>
        auto deref_or_get_dispatch_(precedence_device<1>, T const& val, Int_Type index)
                ->decltype( std::declval<const T&>()[0] )
        {
                return val[index];
        }

        template<class T, class Int_Type>
        auto deref_or_get(T const& val, Int_Type index){
                return deref_or_get_dispatch_(precedence_device<1>{}, val, index);
        }


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
			for(auto iter = deref_or_get(upper, N-1)+1;iter!=0;){
				--iter;
				if( ! f(iter) )
					continue;
				visit_exclusive_combinations<N-1>(v, f, upper, iter, std::forward<Args>(args)...);
			}
		}
                

                template<class... Vecs, int... Seq, class Array, class F>
                void visit_vector_combinations_detail(F f, std::integer_sequence<int, Seq...>, Array const& ar, Vecs... vecs){
                        //PRINT(detail::to_string(ar));
                        f( vecs[ar[Seq]]...);
                }

#if 0
                template<class... Vecs, class F>
                void visit_vector_combinations(F f, Vecs... vecs){

                        std::vector<int> size_vec;

                        int foldaux[] = {0, (size_vec.emplace_back(vecs.size()-1),0)...};

                        detail::visit_exclusive_combinations<sizeof...(vecs)>(
                                [&, vecs...](auto... idx)mutable{
                                        visit_vector_combinations_detail(
                                                f,
                                                std::make_integer_sequence<int, sizeof...(vecs)>{},
                                                std::array<int, sizeof...(idx)>{idx...},
                                                vecs...);
                                }
                                , detail::true_, size_vec );

                }
#endif


#if 0
int main(){
        std::vector<int> a = {1,1,1};
        std::vector<int> b = {1,2,4};
        std::vector<int> c = {1,3,9};

        visit_vector_combinations( 
                [](auto i, auto j, auto k){
                        PRINT_SEQ((i)(j)(k));
                },
                a, b, c
        );
}
#endif

		
} // namespace detail
} // namespace ps

#endif // PS_DETAIL_VISIT_COMBINATIONS_H
