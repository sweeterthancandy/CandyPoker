#include <iostream>
#include <string>
#include <type_traits>
#include "ps/base/holdem_class_vector.h"
#include "ps/base/frontend.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/detail/cross_product.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/base/holdem_class_range_vector.h"

using namespace ps;


struct class_range_equity_evaluator{
        virtual ~class_range_equity_evaluator()=default;

        virtual std::shared_ptr<equity_breakdown> evaluate(holdem_class_range_vector const& players)const=0;
};
using class_range_equity_evaluator_factory = support::singleton_factory<class_range_equity_evaluator>;

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
                        result->append(*impl_->evaluate( v ));
                }, players);
                return result;
        }
private:
        class_equity_evaluator* impl_;
};

int main(){
        holdem_class_range_vector players;
        players.emplace_back("99+, AJs+, AQo+");
        players.emplace_back("55");
        class_range_equity_evaluator_principal ec;
        std::cout << *ec.evaluate(players) << "\n";

}
