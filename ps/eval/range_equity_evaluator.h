#ifndef PS_EVAL_RANGE_EQUITY_EVALUATOR_H
#define PS_EVAL_RANGE_EQUITY_EVALUATOR_H

#include "ps/support/singleton_factory.h"
#include "ps/base/range.h"
#include "ps/eval/equity_breakdown.h"


namespace ps{


struct range_equity_evaluator{
        virtual ~range_equity_evaluator()=default;

        virtual std::shared_ptr<equity_breakdown> evaluate(std::vector<range>const& players)const=0;
};


using range_equity_evaluator_factory = support::singleton_factory<range_equity_evaluator>;

} // ps

#endif // PS_EVAL_RANGE_EQUITY_EVALUATOR_H
