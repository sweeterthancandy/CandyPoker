#ifndef PS_CALCULATOR_H
#define PS_CALCULATOR_H

#include <future>
#include <utility>
#include <thread>
#include <numeric>
#include <mutex>
#include <array>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>

#include "ps/cards_fwd.h"
#include "ps/equity_calc_detail.h"
#include "ps/algorithm.h"
#include "ps/calculator_detail.h"

namespace ps{

        /*
                This servers practically as 
                a compiler firewall.
         */


        struct view_t{
                template<size_t N, class Result>
                view_t make_view( Result const& result){
                        sigma = result.sigma;
                        for(size_t i=0;i!=N;++i){
                                for(size_t j=0;j!=N;++j){
                                        data[i][j] = result.data[i][j];
                                }
                        }
                }
                size_t sigma;
                std::array<
                        std::array<
                                size_t,
                                9
                        >,
                        9
                > data;
        };
        
        template<size_t N_>
        struct sub_calculator_impl{
                constexpr const size_t N = N_;

        };
        struct calculator_impl{
                template<class Calc>
                view_t calculate_hand_equity(std::vector<ps::holdem_id> const& players, Calc& calc){
                        return view_t::make_view< Calc::N >( calc->calculate(players) );
                }
                view_t calculate_hand_equity(std::vector<ps::holdem_id> const& players){
                        switch(players.size()){
                        case 2: return calculate_hand_equity_( players, _2 );
                        case 3: return calculate_hand_equity_( players, _3 );
                        case 4: return calculate_hand_equity_( players, _4 );
                        case 5: return calculate_hand_equity_( players, _5 );
                        case 6: return calculate_hand_equity_( players, _6 );
                        case 7: return calculate_hand_equity_( players, _7 );
                        case 8: return calculate_hand_equity_( players, _8 );
                        case 9: return calculate_hand_equity_( players, _9 );
                        }
                }
        private:
                sub_calculator_impl<2> _2;
                sub_calculator_impl<3> _3;
                sub_calculator_impl<4> _4;
                sub_calculator_impl<5> _5;
                sub_calculator_impl<6> _6;
                sub_calculator_impl<7> _7;
                sub_calculator_impl<8> _8;
                sub_calculator_impl<9> _9;
        };


        struct calculator{
                calculator()
                        :impl_{new calculator_impl{}}
                {}
                ~calculator(){}



                view_t calculate_hand_equity(std::vector<ps::holdem_id> const& players){
                        return impl_->calculate_hand_equity(players);
                }
                bool load(std::string const& name){
                        std::ifstream is(name);
                        if( ! is.is_open() )
                                return false;
                        boost::archive::text_iarchive ia(is);
                        ia >> *impl_;
                        return true;
                }
                bool save(std::string const& name)const{
                        std::ofstream of(name);
                        boost::archive::text_oarchive oa(of);
                        oa << *impl_;
                        return true;
                }

        private:
                std::unique_ptr<calculator_impl> impl_;
        };

        
}

#endif // PS_CALCULATOR_H 
