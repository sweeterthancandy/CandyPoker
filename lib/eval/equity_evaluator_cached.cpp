#include "ps/eval/equity_evaluator_cached.h"

namespace ps{
namespace{
        int reg = ( equity_evaluator_factory::register_<equity_evaluator_cached>("cached"), 0);
} // anon
} // ps
