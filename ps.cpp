#include <iostream>
#include <fstream>
#include <regex>
#include <locale>
#include <map>
#include <codecvt>
#include <type_traits>
#include <functional>

#include "ps/base/range.h"

using namespace ps;


#if 0
namespace detail{
template <class T>
struct is_reference_wrapper : std::false_type {};
template <class U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};
template <class T>
constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;
 
template <class T, class Type, class T1, class... Args>
decltype(auto) INVOKE(Type T::* f, T1&& t1, Args&&... args)
{
    if constexpr (std::is_member_function_pointer_v<decltype(f)>) {
        if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
            return (std::forward<T1>(t1).*f)(std::forward<Args>(args)...);
        else if constexpr (is_reference_wrapper_v<std::decay_t<T1>>)
            return (t1.get().*f)(std::forward<Args>(args)...);
        else
            return ((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...);
    } else {
        static_assert(std::is_member_object_pointer_v<decltype(f)>);
        static_assert(sizeof...(args) == 0);
        if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
            return std::forward<T1>(t1).*f;
        else if constexpr (is_reference_wrapper_v<std::decay_t<T1>>)
            return t1.get().*f;
        else
            return (*std::forward<T1>(t1)).*f;
    }
}
 
template <class F, class... Args>
decltype(auto) INVOKE(F&& f, Args&&... args)
{
      return std::forward<F>(f)(std::forward<Args>(args)...);
}
} // namespace detail
 
template< class F, class... Args>
std::invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) 
  noexcept(std::is_nothrow_invocable_v<F, Args...>)
{
    return detail::INVOKE(std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
        return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
}

template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
        return apply_impl(
                std::forward<F>(f), std::forward<Tuple>(t),
                std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
}

}
#endif

namespace detail{
        template<class First, class... Args>
        auto first_of_pack(First&& first, Args&&... args){
                return first;
        }
}

template<class V, class... Args>
void cross_product( V v, Args&&... args){


        using iter_t = decltype(::detail::first_of_pack(args...).begin());

        struct helper_t{
                helper_t(iter_t i, iter_t e)
                        : proto{i}, iter{i}, end{e}
                {}
                iter_t proto;
                iter_t iter;
                iter_t end;
        };
        std::vector< helper_t > iter;

        int dummy[] = {0,
                ( 
                        iter.emplace_back( args.begin(), args.end()),
                        0)...};

        for(;;){
                v(iter[0].iter, iter[1].iter);

                size_t cursor = 1;
                for(;;){
                        if( ++iter[cursor].iter == iter[cursor].end){
                                if( cursor == 0 )
                                        return;
                                iter[cursor].iter = iter[cursor].proto;
                                --cursor;
                                continue;
                        }
                        break;
                }
        }
}

int main(){
        range hero, villian;
        hero.set_class( holdem_class_decl::parse("KK"));
        hero.set_class( holdem_class_decl::parse("QQ"));
        villian.set_hand( holdem_hand_decl::parse("As2h"));
        villian.set_class( holdem_class_decl::parse("22"));

        cross_product([](auto a, auto b){
                PRINT_SEQ((a.class_())(b.class_()));
                cross_product([](auto a, auto b){
                        PRINT_SEQ((a.decl())(b.decl()));
                },*a, *b);
        },hero, villian);

}
