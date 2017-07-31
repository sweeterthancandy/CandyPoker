#ifndef PS_DETAIL_STATIC_DYNAMIC_H
#define PS_DETAIL_STATIC_DYNAMIC_H

namespace ps{

//constexpr const int _dyn{-1};

// got a compile bug without this
template<int N>
struct _int{
        constexpr const auto __N__(){ return N; }
};
struct _dyn{};

using _1 = _int<1>;
using _2 = _int<2>;
using _3 = _int<3>;
using _4 = _int<4>;
using _5 = _int<5>;
using _6 = _int<6>;
using _7 = _int<7>;
using _8 = _int<8>;
using _9 = _int<9>;
using _10 = _int<10>;

namespace detail{
        
        template<class T>
        struct static_dynamic_traits;

        template<int N>
        struct static_dynamic_traits<_int<N> >{
                template<class T>
                using array_type    = std::array<T, N>;
                template<class T>
                using array_sq_type = std::array<T, N * N>;

                template<class T>
                static auto make_array()     { return array_type<T>{};        }
                template<class T>
                static auto make_sq_array()  { return array_sq_type<T>{};     }

                constexpr bool is_static()   { return true;                   }
                constexpr bool is_dynamic()  { return false;                  }

                static constexpr auto get_n(){ return static_cast<size_t>(N); }
                
        };

        template<>
        struct static_dynamic_traits<_dyn>{
                template<class T>
                using array_type = std::vector<T>;
                template<class T>
                using array_sq_type = std::vector<T>;

                template<class T>
                static auto make_array(size_t n)     { return array_type<T>(n);         }
                template<class T>
                static auto make_sq_array(size_t n)  { return array_sq_type<T>(n*n); }
                
                constexpr bool is_static()           { return false; }
                constexpr bool is_dynamic()          { return true; }
                
                static constexpr auto get_n(size_t n){ return n; }
        };

        template<class Decl, class T, class... Args>
        auto make_array(Args&&... args){
                return static_dynamic_traits<Decl>::template make_array<T>(std::forward<Args>(args)...);
        }
        template<class Decl, class T, class... Args>
        auto make_sq_array(Args&&... args){
                return static_dynamic_traits<Decl>::template make_sq_array<T>(std::forward<Args>(args)...);
        }
        template<class Decl, class... Args>
        auto get_n(Args&&... args){
                return static_dynamic_traits<Decl>::get_n(std::forward<Args>(args)...);
        }


} // detail
} // ps

#endif // PS_DETAIL_STATIC_DYNAMIC_H
