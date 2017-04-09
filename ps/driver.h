#ifndef PS_DRIVER_H
#define PS_DRIVER_H

#include "eval.h"

#include <ostream>
#include <string>

#include <boost/format.hpp>


#include "detail/print.h"
#include "detail/void_t.h"

namespace ps{

struct driver{
        std::uint32_t eval_5(std::string const& str)const{
                assert( str.size() == 10 && "precondition failed");
                std::vector<long> hand;
                for(size_t i=0;i!=10;i+=2){
                        hand.push_back(traits_.make(str[i], str[i+1]) );
                }
                return eval_.eval_5(hand);
        }
private:
        eval eval_;
        card_traits traits_;
};

} // namespace ps

#endif // PS_DRIVER_H
