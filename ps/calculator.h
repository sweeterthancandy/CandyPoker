#ifndef PS_CALCULATOR_H
#define PS_CALCULATOR_H

#include <string>
#include <memory>

#include "ps/cards_fwd.h"
#include "ps/calculator_view.h"

namespace ps{
        
        using view_t = detailed_view_type;

        struct  calculator_impl;

        struct calculater{
                calculater();
                ~calculater();



                // how about some sugar
                template<class Players_Type>
                view_t calculate_hand_equity(Players_Type const& players){
                        return calculate_hand_equity_( detail::make_array_view(players) );
                }
                template<class Players_Type>
                view_t calculate_class_equity(Players_Type const& players){
                        return calculate_class_equity_(detail::make_array_view(players));
                }


                bool load(std::string const& name);
                bool save(std::string const& name)const;

        private:
                view_t calculate_hand_equity_(detail::array_view<holdem_id> const& players);
                view_t calculate_class_equity_(detail::array_view<holdem_id> const& players);

                std::unique_ptr<calculator_impl> impl_;
        };

        
}

#endif // PS_CALCULATOR_H 
