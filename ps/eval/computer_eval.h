#ifndef PS_EVAL_COMPUTER_EVAL_H
#define PS_EVAL_COMPUTER_EVAL_H

#include "ps/eval/equity_evaluator_principal.h"

namespace ps{

#if 0
struct eval_computer : computer{
        virtual Eigen::MatrixXd compute(computation_context const& ctx, instruction_list const& instr_list)override{
                instruction_list my_instr_list = instruction_list_deep_copy(instr_list);
                auto card_instr_list = transform_cast_to_card_eval(my_instr_list);
                
                auto agg = std::make_shared<equity_breakdown_matrix_aggregator>(ctx.NumPlayers());
                for(auto const& instr : card_instr_list ){
                        auto result = eval_.evaluate(instr.get_vector());
                        agg->append_matrix(*result, instr.get_matrix());
                }
                return agg;
        }
private:
        equity_evaulator_principal eval_;
};
#endif

} // ps

#endif // PS_EVAL_COMPUTER_EVAL_H
