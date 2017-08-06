#include "ps/eval/equity_evaluator_principal.h"

namespace ps{
namespace {
        int ret = ( equity_evaluator_factory::register_<equity_evaulator_principal>("principal"), 0);
}
} // ps
