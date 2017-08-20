#include "ps/eval/class_equity_evaluator_quick.h"

namespace ps{
namespace {
        int reg = ( class_equity_evaluator_factory::register_<class_equity_evaluator_quick>("quick"), 0);
} // anon
} // ps
