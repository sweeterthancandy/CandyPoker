#ifndef PS_ALGORITHM_H
#define PS_ALGORITHM_H

#include <tuple>
#include <vector>

#include "ps/cards.h"

namespace ps{

std::tuple<
        std::vector<int>,
        std::vector<ps::holdem_id>
> permutate_for_the_better( std::vector<ps::holdem_id> const& players );

} // ps

#endif // PS_ALGORITHM_H
