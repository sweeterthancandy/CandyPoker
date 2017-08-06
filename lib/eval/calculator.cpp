#include "ps/eval/calculator.h"

#include "ps/base/cards_fwd.h"
#include "ps/eval/equity_calc_detail.h"
#include "ps/base/algorithm.h"
#include "ps/eval/calculator_detail.h"

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

#if 0
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#endif

#if 0
namespace ps{

        /*
                This servers practically as 
                a compiler firewall.
         */


        struct calculator_impl{

                using view_t = detailed_view_type;

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
                view_t calculate_hand_equity(support::array_view<ps::holdem_id> const& players){
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
                view_t calculate_class_equity(support::array_view<ps::holdem_class_id> const& players){
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
                void append(calculator_impl const& that){
                        _2_.append(that._2_);
                        _3_.append(that._3_);
                        _4_.append(that._4_);
                        _5_.append(that._5_);
                        _6_.append(that._6_);
                        _7_.append(that._7_);
                        _8_.append(that._8_);
                        _9_.append(that._9_);
                        _c_2_.append(that._c_2_);
                        _c_3_.append(that._c_3_);
                        _c_4_.append(that._c_4_);
                        _c_5_.append(that._c_5_);
                        _c_6_.append(that._c_6_);
                        _c_7_.append(that._c_7_);
                        _c_8_.append(that._c_8_);
                        _c_9_.append(that._c_9_);
                }
                void json_dump(std::ostream& ostr)const{
                        #if 0
                        using bpt = boost::property_tree;
                        bpt::ptree root;
                        root.add("2_player_call"
                        #endif

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
        calculater::calculater(calculater&& that)=default;
        calculater& calculater::operator=(calculater&& that)=default;


        calculater::view_t calculater::calculate_hand_equity_(support::array_view<holdem_id> const& players){
                return impl_->calculate_hand_equity(players);
        }
        calculater::view_t calculater::calculate_class_equity_(support::array_view<holdem_id> const& players){
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

        void calculater::append(calculater const& that){
                impl_->append( *that.impl_);
        }
        void calculater::json_dump(std::ostream& ostr)const{
                impl_->json_dump( ostr );
        }

} // ps
#endif
