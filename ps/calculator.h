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
                        return calculate_hand_equity_( detail::make_array_view(players) );
                }
                template<class Players_Type>
                view_t calculate_class_equity(Players_Type const& players){
                        return calculate_class_equity_(detail::make_array_view(players));
                }


                bool load(std::string const& name);
                bool save(std::string const& name)const;
                void append(calculater const& that);

                // It's nice to have nice things
                void json_dump(std::ostream& ostr)const;

        private:
                view_t calculate_hand_equity_(detail::array_view<holdem_id> const& players);
                view_t calculate_class_equity_(detail::array_view<holdem_id> const& players);

                std::unique_ptr<calculator_impl> impl_;
        };

        
}

#endif // PS_CALCULATOR_H 
