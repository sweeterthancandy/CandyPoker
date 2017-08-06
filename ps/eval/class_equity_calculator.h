#ifndef PS_EVAL_CLASS_EQUITY_EVALUATOR_H
#define PS_EVAL_CLASS_EQUITY_EVALUATOR_H

#include "ps/support/singleton_factory.h"
#include "ps/eval/equity_eval_result.h"

namespace ps{


struct class_equity_evaluator{
        virtual ~class_equity_evaluator()=default;

        virtual equity_eval_result evaluate(std::vector<holdem_id> const& players)const=0;
};


using equity_evaluator_factory = support::singleton_factory<class_equity_evaluator>;

} // ps

#endif // PS_EVAL_CLASS_EQUITY_EVALUATOR_H
