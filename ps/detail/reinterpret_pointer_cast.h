#ifndef PS_DETAIL_REINTERPRET_POINTER_CAST_H
#define PS_DETAIL_REINTERPRET_POINTER_CAST_H

#include <memory>

namespace ps{
namespace detail{

template< class T, class U > 
std::shared_ptr<T> reinterpret_pointer_cast( const std::shared_ptr<U>& r ) noexcept
{
        auto p = reinterpret_cast<typename std::shared_ptr<T>::element_type*>(r.get());
        return std::shared_ptr<T>(r, p);
}

} // end namespace detail
} // end namespace ps

#endif // PS_DETAIL_REINTERPRET_POINTER_CAST_H
