#ifndef PS_CALCULATOR_H
#define PS_CALCULATOR_H

#include <string>
#include <memory>
#include <array>

#include "ps/base/cards_fwd.h"
#include "ps/eval/calculator_view.h"
#include "ps/eval/aggregator.h"

namespace ps{
        
        
        struct  calculator_impl;

        struct calculater{
        
                using view_t = detailed_view_type;

                // rule of 6
                calculater();
                ~calculater();
                calculater(const calculater& that)=delete;
                calculater(calculater&& that);
                calculater& operator=(const calculater& that)=delete;
                calculater& operator=(calculater&& that);



                // how about some sugar
                template<class Players_Type>
                view_t calculate_hand_equity(Players_Type const& players){
                        return calculate_hand_equity_( support::make_array_view(players) );
                }
                template<class Players_Type>
                view_t calculate_class_equity(Players_Type const& players){
                        return calculate_class_equity_(support::make_array_view(players));
                }
                template<class... Args>
                view_t calculate_hand_equity(Args... args){
                        // should this be a cast or not?
                        std::array<ps::holdem_id, sizeof...(args)> aux{ static_cast<ps::holdem_id>(args)... };
                        return calculate_hand_equity_(support::make_array_view(aux));
                }
                template<class... Args>
                view_t calculate_class_equity(Args... args){
                        // should this be a cast or not?
                        std::array<ps::holdem_class_id, sizeof...(args)> aux{ static_cast<ps::holdem_class_id>(args)... };
                        return calculate_class_equity_(support::make_array_view(aux));
                }



                bool load(std::string const& name);
                bool save(std::string const& name)const;
                void append(calculater const& that);

                // It's nice to have nice things
                void json_dump(std::ostream& ostr)const;

        private:
                view_t calculate_hand_equity_(support::array_view<holdem_id> const& players);
                view_t calculate_class_equity_(support::array_view<holdem_id> const& players);

                std::unique_ptr<calculator_impl> impl_;
        };

        
}

#endif // PS_CALCULATOR_H 
