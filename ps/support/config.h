#ifndef PS_SUPPORT_CONFIG_H
#define PS_SUPPORT_CONFIG_H

#include <boost/config.hpp>

#if BOOST_WINDOWS
        #define PS_UNREACHABLE() _assume(0)
#else
        #define PS_UNREACHABLE() __builtin_unreachable()
#endif // BOOST_WINDOWS

#endif // PS_SUPPORT_CONFIG_H 
