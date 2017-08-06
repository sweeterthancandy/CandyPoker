#ifndef PS_EVAL_EVALUATOR_H
#define PS_EVAL_EVALUATOR_H

#include <array>

#include "ps/base/cards.h"
#include "ps/support/singleton_factory.h"
#include "ps/eval/rank_decl.h"

namespace ps{

struct evaluator{
        virtual ~evaluator()=default;
        //virtual ranking const& rank(std::vector<long> const& cards)const;
        virtual ranking_t rank(long a, long b, long c, long d, long e)const=0;
        virtual ranking_t rank(long a, long b, long c, long d, long e, long f)const=0;
        virtual ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const=0;
};

using evaluator_factory = support::singleton_factory<evaluator>;

} // namespace ps

#endif // #ifndef PS_EVAL_EVALUATOR_H