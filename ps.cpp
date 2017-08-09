#include <iostream>
#include <string>
#include <type_traits>
#include "ps/base/holdem_class_vector.h"
#include "ps/base/frontend.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/detail/cross_product.h"
#include "ps/eval/equity_breakdown_matrix.h"

using namespace ps;

struct holdem_class_range : std::vector<ps::holdem_class_id>{
        template<
                class... Args,
                class = std::enable_if_t< ! std::is_constructible<std::string, Args...>::value  >
        >
        holdem_class_range(Args&&... args)
        : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
        {}
        holdem_class_range(std::string const& item){
                this->parse(item);
        }
        friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_class_decl::get(id).to_string();
                                                 } );
        }
        void parse(std::string const& item){
                auto rep = expand(frontend::parse(item));
                boost::copy( rep.to_class_vector(), std::back_inserter(*this)); 
        }
};

struct holdem_class_range_vector : std::vector<holdem_class_range>{
        template<class... Args>
        holdem_class_range_vector(Args&&... args)
        : std::vector<holdem_class_range>{std::forward<Args>(args)...}
        {}
        friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range_vector const& self){
                return ostr << detail::to_string(self);
        }
};

int main(){
        holdem_class_range_vector players;
        players.emplace_back("99+, AJs+, AQo+");
        players.emplace_back("55");
        #if 0
        players.push_back(holdem_class_decl::parse("A2o"));
        players.push_back(holdem_class_decl::parse("88"));
        players.push_back(holdem_class_decl::parse("QJs"));

        auto const& ec = class_equity_evaluator_factory::get("proc");
        std::cout << *ec.evaluate(players) << "\n";

        #endif
        auto const& ec = class_equity_evaluator_factory::get("proc");
        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
        detail::cross_product_vec([&](auto const& vec){
                std::vector<holdem_class_id> v;
                for( auto i : vec )
                        v.push_back(*i);
                result->append(*ec.evaluate( v ));
        }, players);
        std::cout << *result << "\n";
        PRINT( players );

}
