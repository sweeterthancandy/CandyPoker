#include "ps/calculator.h"
#include "ps/frontend.h"
#include "ps/tree.h"

#include <type_traits>
#include <functional>
#include <iostream>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>

using namespace ps;

namespace ps{

        struct aggregator{
                void append(view_t const& view){
                        if( n_ == 0 ){
                                PRINT(view.n());
                                std::cout << std::flush;
                                n_ = view.n();
                                data_.resize( n_ * n_ );
                        }
                        sigma_ += view.sigma();
                        for(size_t i=0;i!=n_;++i){
                                for(size_t j=0;j!=n_;++j){
                                        data_access(i, j) += view.player(i).nwin(j);
                                }
                        }
                }
                view_t make_view(){
                        std::vector<int> perm;
                        for(size_t i=0;i!=n_;++i)
                                perm.emplace_back(i);
                        return view_t{
                                n_,
                                sigma_,
                                detail::array_view<size_t>{ data_},
                                std::move(perm)
                        };
                }
        private:
                size_t& data_access(size_t i,size_t j){
                        return data_[i * n_ + j];
                }
                size_t n_=0;
                size_t sigma_ =0;
                std::vector< size_t > data_;
        };

        /*
                poker_stove AKs TT QQ
                poker_stove --cache new_cache.bin AKs TT
         */

        struct pretty_printer{
                void operator()(std::ostream& ostr, view_t result, std::vector<std::string> const& players){
                        std::vector<
                                std::vector<std::string>
                        > lines;

                        lines.emplace_back();
                        lines.back().emplace_back("range");
                        lines.back().emplace_back("equity");
                        lines.back().emplace_back("wins");
                        lines.back().emplace_back("draws");
                        lines.back().emplace_back("lose");
                        lines.back().emplace_back("sigma");
                        lines.emplace_back();
                        lines.back().push_back("__break__");
                        for( size_t i=0;i!=players.size();++i){
                                auto pv{ result.player(i) };
                                lines.emplace_back();

                                lines.back().emplace_back( boost::lexical_cast<std::string>(players[i]) );
                                lines.back().emplace_back( boost::lexical_cast<std::string>(pv.equity()) );
                                lines.back().emplace_back( boost::lexical_cast<std::string>(pv.win()));
                                lines.back().emplace_back( boost::lexical_cast<std::string>(pv.draw()));
                                lines.back().emplace_back( boost::lexical_cast<std::string>(pv.lose()));

                                lines.back().emplace_back( boost::lexical_cast<std::string>(pv.sigma()) );
                        }
                        
                        std::vector<size_t> widths(6,0);
                        for( auto const& line : lines){
                                for(size_t i=0;i!=line.size();++i){
                                        widths[i] = std::max(line[i].size(), widths[i]);
                                }
                        }
                        auto pad{ [](auto const& s, size_t w){
                                size_t padding{ w - s.size()};
                                size_t left_pad{padding/2};
                                size_t right_pad{padding - left_pad};
                                std::string ret;
                                if(left_pad)
                                       ret += std::string(left_pad,' ');
                                ret += s;
                                if(right_pad)
                                       ret += std::string(right_pad,' ');
                                return std::move(ret);
                        }};
                        for( auto const& line : lines){
                                if( line.size() >= 1 && line[0] == "__break__" ){
                                        for(size_t i=0;i!=widths.size();++i){
                                                if( i != 0 )
                                                        std::cout << "-+-";
                                                std::cout << std::string(widths[i],'-');
                                        }
                                } else{
                                        for(size_t i=0;i!=line.size();++i){
                                                if( i != 0 )
                                                        std::cout << " | ";
                                                std::cout << pad(line[i], widths[i]);
                                        }
                                }
                                std::cout << "\n";
                        }
                }
        };

        int pokerstove_driver(int argc, char** argv){
                using namespace ps::frontend;

                calculater calc;
                #if 0
                ec.load("cache.bin");
                cec.load("hc_cache.bin");
                #endif

                std::vector<std::string> players_s;
                std::vector<frontend::range> players;
                for(int i=1;i < argc; ++i){
                        players_s.push_back( argv[i] );
                        players.push_back( frontend::parse(argv[i]));
                }

                double sigma{0};
                size_t factor{0};
                tree_range root{ players };
                aggregator agg;
                for( auto const& c : root.children ){

                        // this means it's a class vs class evaulation
                        if( c.opt_cplayers.size() != 0 ){
                                auto ret{ calc.calculate_class_equity( c.opt_cplayers ) };
                                agg.append(ret);
                        } else{
                                for( auto const& d : c.children ){
                                        auto ret{ calc.calculate_hand_equity( d.players ) };
                                        agg.append(ret);
                                }
                        }

                }

                pretty_printer{}(std::cout, agg.make_view(), players_s);

                return 0;
        }
} // anon

int main(int argc, char** argv){
        return pokerstove_driver(argc, argv);
}
