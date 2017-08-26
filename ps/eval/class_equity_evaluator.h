#ifndef PS_EVAL_CLASS_EQUITY_EVALUATOR_H
#define PS_EVAL_CLASS_EQUITY_EVALUATOR_H

#include "ps/support/singleton_factory.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/eval/equity_breakdown.h"
#include "ps/eval/holdem_eval_cache.h"
#include "ps/eval/holdem_class_eval_cache.h"


namespace ps{


struct class_equity_evaluator{
        virtual ~class_equity_evaluator()=default;

        virtual std::shared_ptr<equity_breakdown> evaluate(holdem_class_vector const& players)const=0;

        // maybe no-op
        virtual void inject_class_cache(std::shared_ptr<holdem_class_eval_cache> ptr){}
        virtual void inject_cache(std::shared_ptr<holdem_eval_cache> ptr){}
};


using class_equity_evaluator_factory = support::singleton_factory<class_equity_evaluator>;

} // ps

#endif // PS_EVAL_CLASS_EQUITY_EVALUATOR_H
