#ifndef PS_EVAL_CLASS_RANGE_EQUITY_EVALUATOR_PRINCIPAL_H
#define PS_EVAL_CLASS_RANGE_EQUITY_EVALUATOR_PRINCIPAL_H

#include "ps/eval/class_range_equity_evaluator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/detail/cross_product.h"

namespace ps{

struct class_range_equity_evaluator_principal : class_range_equity_evaluator{
        class_range_equity_evaluator_principal()
                :impl_( &class_equity_evaluator_factory::get("proc") )
        {}
        std::shared_ptr<equity_breakdown> evaluate(holdem_class_range_vector const& players)const override{
                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
                detail::cross_product_vec([&](auto const& vec){
                        std::vector<holdem_class_id> v;
                        for( auto i : vec )
                                v.push_back(*i);
                        result->append(*impl_->evaluate_class( v ));
                }, players);
                return result;
        }
private:
        class_equity_evaluator* impl_;
};
} // ps
#endif //PS_EVAL_CLASS_RANGE_EQUITY_EVALUATOR_PRINCIPAL_H 
