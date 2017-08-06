#ifndef PS_EVAL_CLASS_EQUITY_EVALUATOR_H
#define PS_EVAL_CLASS_EQUITY_EVALUATOR_H

#include "ps/support/singleton_factory.h"
#include "ps/base/hand_vector.h"
#include "ps/eval/equity_breakdown.h"


namespace ps{


struct class_equity_evaluator{
        virtual ~class_equity_evaluator()=default;

        virtual std::shared_ptr<equity_breakdown> evaluate(holdem_class_vector const& players)const=0;
};


using class_equity_evaluator_factory = support::singleton_factory<class_equity_evaluator>;

} // ps

#endif // PS_EVAL_CLASS_EQUITY_EVALUATOR_H
