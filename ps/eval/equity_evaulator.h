#ifndef PS_EVAL_EQUITY_EVALUATOR_H
#define PS_EVAL_EQUITY_EVALUATOR_H


#include "ps/support/singleton_factory.h"

#include "ps/eval/equity_eval_result.h"

namespace ps{
        


struct equity_evaluator{
        virtual ~equity_evaluator()=default;

        virtual equity_eval_result evaluate(std::vector<holdem_id> const& players)const=0;
};


using equity_evaluator_factory = support::singleton_factory<equity_evaluator>;

} // ps

#endif // #ifndef PS_EVAL_EQUITY_EVALUATOR_H
