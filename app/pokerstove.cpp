#include "ps/tree.h"
#include "ps/frontend.h"
#include "ps/heads_up.h"

#include <type_traits>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>

using namespace ps;

namespace{
        int pokerstove_driver(int argc, char** argv){
                using namespace ps::frontend;
                equity_cacher ec;
                class_equity_cacher cec{ec};
                ec.load("cache.bin");
                cec.load("hc_cache.bin");
                std::vector<std::string> players_s;
                std::vector<frontend::range> players;
                for(int i=1;i < argc; ++i){
                        players_s.push_back( argv[i] );
                        players.push_back( frontend::parse(argv[i]));
                }
                
                double sigma{0};
                size_t factor{0};
                tree_range root{ players };
                hu_fresult_t agg;
                for( auto const& c : root.children ){

                        if( c.opt_cplayers.size() == 2 ){
                                auto ret{ cec.visit_boards( c.opt_cplayers ) };
                                agg.append(ret);
                        } else{
                                for( auto const& d : c.children ){
                                        auto ret{ ec.visit_boards( d.players ) };
                                        agg.append(ret);
                                }
                        }

                }

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
                lines.emplace_back();
                lines.back().emplace_back( boost::lexical_cast<std::string>(players_s[0]) );
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.equity()) );
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.win ));
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.draw ));
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.lose ));
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.sigma()) );
                lines.emplace_back();
                lines.back().emplace_back( boost::lexical_cast<std::string>(players_s[1]) );
                lines.back().emplace_back( boost::lexical_cast<std::string>(1.0-agg.equity()) );
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.lose ));
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.draw ));
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.win ));
                lines.back().emplace_back( boost::lexical_cast<std::string>(agg.sigma()) );
                
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


                

                return 0;
        }
} // anon

int main(int argc, char** argv){
        return pokerstove_driver(argc, argv);
}
