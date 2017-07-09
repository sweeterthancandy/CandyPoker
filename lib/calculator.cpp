#include "ps/calculator.h"

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


        struct calculator_impl{
                calculator_impl():
                        ecd_{},
                        _2_{&ecd_},
                        _3_{&ecd_},
                        _4_{&ecd_},
                        _5_{&ecd_},
                        _6_{&ecd_},
                        _7_{&ecd_},
                        _8_{&ecd_},
                        _9_{&ecd_},
                        _c_2_{&_2_},
                        _c_3_{&_3_},
                        _c_4_{&_4_},
                        _c_5_{&_5_},
                        _c_6_{&_6_},
                        _c_7_{&_7_},
                        _c_8_{&_8_},
                        _c_9_{&_9_}
                {
                }
                view_t calculate_hand_equity(detail::array_view<ps::holdem_id> const& players){
                        switch(players.size()){
                        case 2: return _2_.calculate(players);
                        case 3: return _3_.calculate(players);
                        case 4: return _4_.calculate(players);
                        case 5: return _5_.calculate(players);
                        case 6: return _6_.calculate(players);
                        case 7: return _7_.calculate(players);
                        case 8: return _8_.calculate(players);
                        case 9: return _9_.calculate(players);
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("bad number of players"));
                        }
                }
                view_t calculate_class_equity(detail::array_view<ps::holdem_class_id> const& players){
                        switch(players.size()){
                        case 2: return _c_2_.calculate(players);
                        case 3: return _c_3_.calculate(players);
                        case 4: return _c_4_.calculate(players);
                        case 5: return _c_5_.calculate(players);
                        case 6: return _c_6_.calculate(players);
                        case 7: return _c_7_.calculate(players);
                        case 8: return _c_8_.calculate(players);
                        case 9: return _c_9_.calculate(players);
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("bad number of players"));
                        }
                }
                template<class Archive>
                void serialize(Archive& ar, unsigned int){
                        ar & _2_;
                        ar & _3_;
                        ar & _4_;
                        ar & _5_;
                        ar & _6_;
                        ar & _7_;
                        ar & _8_;
                        ar & _9_;
                        
                        ar & _c_2_;
                        ar & _c_3_;
                        ar & _c_4_;
                        ar & _c_5_;
                        ar & _c_6_;
                        ar & _c_7_;
                        ar & _c_8_;
                        ar & _c_9_;
                }
        private:
                equity_calc_detail ecd_;

                detail::basic_calculator_N<2> _2_;
                detail::basic_calculator_N<3> _3_;
                detail::basic_calculator_N<4> _4_;
                detail::basic_calculator_N<5> _5_;
                detail::basic_calculator_N<6> _6_;
                detail::basic_calculator_N<7> _7_;
                detail::basic_calculator_N<8> _8_;
                detail::basic_calculator_N<9> _9_;
                
                detail::basic_class_calculator_N<2> _c_2_;
                detail::basic_class_calculator_N<3> _c_3_;
                detail::basic_class_calculator_N<4> _c_4_;
                detail::basic_class_calculator_N<5> _c_5_;
                detail::basic_class_calculator_N<6> _c_6_;
                detail::basic_class_calculator_N<7> _c_7_;
                detail::basic_class_calculator_N<8> _c_8_;
                detail::basic_class_calculator_N<9> _c_9_;
        };
} // ps

namespace ps{

        
        calculater::calculater()
                :impl_{new calculator_impl{}}
        {}
        calculater::~calculater()=default;


        view_t calculater::calculate_hand_equity_(detail::array_view<holdem_id> const& players){
                return impl_->calculate_hand_equity(players);
        }
        view_t calculater::calculate_class_equity_(detail::array_view<holdem_id> const& players){
                return impl_->calculate_class_equity(players);
        }

        bool calculater::load(std::string const& name){
                std::ifstream is(name);
                if( ! is.is_open() )
                        return false;
                boost::archive::text_iarchive ia(is);
                ia >> *impl_;
                return true;
        }
        bool calculater::save(std::string const& name)const{
                std::ofstream of(name);
                boost::archive::text_oarchive oa(of);
                oa << *impl_;
                return true;
        }

} // ps
