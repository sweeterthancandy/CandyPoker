#ifndef PS_DETAIL_VOID_T_H
#define PS_DETAIL_VOID_T_H

namespace ps{
namespace detail{
template<typename... Ts> struct make_void { typedef void type;};
template<typename... Ts> using void_t = typename make_void<Ts...>::type;
}
} // namespace ps

#endif // PS_DETAIL_VOID_T_H
