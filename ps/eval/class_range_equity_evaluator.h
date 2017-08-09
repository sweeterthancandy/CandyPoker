#ifndef PS_EVALCLASS_RANGE_EQUITY_EVALUATOR
#define PS_EVALCLASS_RANGE_EQUITY_EVALUATOR

#include <memory>

#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/equity_breakdown.h"
#include "ps/support/singleton_factory.h"

namespace ps{

struct class_range_equity_evaluator{
        virtual ~class_range_equity_evaluator()=default;

        virtual std::shared_ptr<equity_breakdown> evaluate(holdem_class_range_vector const& players)const=0;
};
using class_range_equity_evaluator_factory = support::singleton_factory<class_range_equity_evaluator>;

} // ps

#endif // PS_EVALCLASS_RANGE_EQUITY_EVALUATOR
