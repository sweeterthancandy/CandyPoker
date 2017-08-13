#ifndef PS_BASE_HOLDEM_CLASS_RANGE_H
#define PS_BASE_HOLDEM_CLASS_RANGE_H

#include <vector>

#include "ps/base/cards.h"
#include "ps/base/frontend.h"
#include "ps/detail/print.h"

namespace ps{

        struct holdem_class_range : std::vector<ps::holdem_class_id>{
                template<
                        class... Args,
                        class = std::enable_if_t< ! std::is_constructible<std::string, Args...>::value  >
                >
                holdem_class_range(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                holdem_class_range(std::string const& item);
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range const& self);
                void parse(std::string const& item);
        };
} // ps
#endif //  PS_BASE_HOLDEM_CLASS_RANGE_H
