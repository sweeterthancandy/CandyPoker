#ifndef PS_EVAL_RANGE_EQUITY_EVALUATOR_PRINCIPAL_H
#define PS_EVAL_RANGE_EQUITY_EVALUATOR_PRINCIPAL_H

#include "ps/detail/cross_product.h"
#include "ps/base/range.h"
#include "ps/eval/range_equity_evaluator.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"

namespace ps{

struct range_equity_evaluator_principal : range_equity_evaluator{

        std::shared_ptr<equity_breakdown> evaluate(std::vector<holdem_range>const& players)const override{
                auto const& ec = equity_evaluator_factory::get("cached");
                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(2);

                detail::cross_product_vec([&](auto const& byclass){
                        detail::cross_product_vec([&](auto const& byhand){
                                holdem_hand_vector v;
                                for( auto iter : byhand ){
                                        v.push_back( (*iter).hand().id() );
                                }
                                result->append(*ec.evaluate( v ));
                        }, byclass);
                }, players);
                return result;
        }
};

}

#endif // PS_EVAL_RANGE_EQUITY_EVALUATOR_PRINCIPAL_H
