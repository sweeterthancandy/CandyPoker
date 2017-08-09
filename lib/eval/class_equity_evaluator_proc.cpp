#include "ps/eval/class_equity_evaluator_proc.h"

namespace ps{
namespace {
        int reg = ( class_equity_evaluator_factory::register_<class_equity_evaluator_proc>("proc"), 0);
} // anon
} // ps
