#ifndef PS_EVAL_CLASS_EQUITY_EVALUATOR_QUICK_H
#define PS_EVAL_CLASS_EQUITY_EVALUATOR_QUICK_H

#include "ps/eval/evaluator_7_card_map.h"
#include "ps/eval/equity_evaluator_principal.h"
#include "ps/eval/class_equity_evaluator_principal.h"

namespace ps{
        struct equity_evaulator_quick
                : equity_evaulator_principal_tpl<evaluator_7_card_map>
        {
                equity_evaulator_quick()
                {
                        impl_ = new evaluator_7_card_map;
                }
        };

        struct class_equity_evaluator_quick
                : class_equity_evaluator_principal_tpl<equity_evaulator_quick>
        {
                class_equity_evaluator_quick()
                {
                        impl_ = new equity_evaulator_quick;
                }
        };
} // ps

#endif // PS_EVAL_CLASS_EQUITY_EVALUATOR_QUICK_H
