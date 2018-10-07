#ifndef PS_EVAL_EVALUATOR_H
#define PS_EVAL_EVALUATOR_H

#include <array>

#include "ps/base/cards.h"
#include "ps/support/singleton_factory.h"
#include "ps/eval/rank_decl.h"

namespace ps{

/*
        At the lowest level we have the 5,6, and 7 card evaluator. This maps 7 distinct
        cards from the desk, to number in [0,M], such that 0 is a royal flush, and
        M is 7 high.
                As an implementation detail, a 5 card evaluation isn't too complicated
        because we can just split the card into two groups, though without a flush
        and those with a flush, and then each of these can be represented as an array
        lookup.
                But 6 and 7 card evaluations are more complicated because there's too
        much memory to create a canonical 7 card array lookup, by a 7 card look up is
                        Inf { rank(v) : v is 5 cards from w },
        however this isn't too efficent
 */
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
