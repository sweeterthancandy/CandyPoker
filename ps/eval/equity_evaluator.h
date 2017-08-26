#ifndef PS_EVAL_EQUITY_EVALUATOR_H
#define PS_EVAL_EQUITY_EVALUATOR_H


#include "ps/base/cards_fwd.h"
#include "ps/support/singleton_factory.h"

#include "ps/eval/equity_breakdown.h"
#include "ps/eval/holdem_eval_cache.h"

namespace ps{
        


struct equity_evaluator : virtual cache_injectable
{
        virtual ~equity_evaluator()=default;

        virtual std::shared_ptr<equity_breakdown> evaluate(std::vector<holdem_id> const& players)const=0;

};


using equity_evaluator_factory = support::singleton_factory<equity_evaluator>;

} // ps

#endif // #ifndef PS_EVAL_EQUITY_EVALUATOR_H
