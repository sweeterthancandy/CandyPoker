#include "ps/eval/better_class_equity_evaluator.h"

namespace ps{
namespace{
        void do_reg(){
                class_equity_evaluator_factory::register_<better_class_equity_evaluator>("better");
                auto maker = [](){
                        auto null_deleter = [](auto){};
                        auto ptr = &class_equity_evaluator_factory::get("better");
                        auto cast_ptr = dynamic_cast<equity_evaluator*>(ptr);
                        return std::shared_ptr<equity_evaluator>(cast_ptr, null_deleter);
                };
                equity_evaluator_factory::register_<better_class_equity_evaluator>("better", maker);
        }

        int reg = ( do_reg(), 0); 
} // anon
} // ps
