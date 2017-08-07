#include "ps/eval/range_equity_evaluator_principal.h"

namespace ps{
namespace {
        int reg = ( range_equity_evaluator_factory::register_<range_equity_evaluator_principal>("principal"), 0);
} // anon
} // ps
