#include "ps/eval/equity_evaulator.h"

namespace ps{
namespace {
        int ret = ( equity_evaluator_factory::register_<equity_evaulator_impl>(), 0);
}
} // ps
