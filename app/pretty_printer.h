#ifndef PS_APP_PRETTY_PRINTER_H
#define PS_APP_PRETTY_PRINTER_H

namespace ps{
        inline
        void pretty_printer(std::ostream& ostr, equity_breakdown const& breakdown , std::vector<std::string> const& players){
                std::vector<
                        std::vector<std::string>
                > lines;

                lines.emplace_back();
                lines.back().emplace_back("range");
                lines.back().emplace_back("equity");
                lines.back().emplace_back("wins");
                //lines.back().emplace_back("draws");
                #if 1
                for(size_t i=0; i != players.size() -1;++i){
                        lines.back().emplace_back("draw_"+ boost::lexical_cast<std::string>(i+1));
                }
                
                #endif
                lines.back().emplace_back("draw equity");
                lines.back().emplace_back("lose");
                lines.back().emplace_back("sigma");
                lines.emplace_back();
                lines.back().push_back("__break__");
                for( size_t i=0;i!=players.size();++i){
                        auto const& pv =  breakdown.player(i) ;
                        lines.emplace_back();

                        lines.back().emplace_back( boost::lexical_cast<std::string>(players[i]) );
                        lines.back().emplace_back( str(boost::format("%.4f%%") % (pv.equity() * 100)));
                        /*
                                draw_equity = \sum_i=1..n win_{i}/i
                        */
                        for(size_t i=0; i != players.size(); ++i ){
                                lines.back().emplace_back( boost::lexical_cast<std::string>(pv.nwin(i)));
                        }

                        auto draw_sigma = 
                                (pv.equity() - static_cast<double>(pv.win())/pv.sigma())*pv.sigma();
                        lines.back().emplace_back( str(boost::format("%.2f%%") % ( draw_sigma )));
                        lines.back().emplace_back( boost::lexical_cast<std::string>(pv.lose()) );
                        lines.back().emplace_back( boost::lexical_cast<std::string>(pv.sigma()) );
                }
                
                std::vector<size_t> widths(lines.back().size(),0);
                for( auto const& line : lines){
                        for(size_t i=0;i!=line.size();++i){
                                widths[i] = std::max(line[i].size(), widths[i]);
                        }
                }
                auto pad= [](auto const& s, size_t w){
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
                };
                for( auto const& line : lines){
                        if( line.size() >= 1 && line[0] == "__break__" ){
                                for(size_t i=0;i!=widths.size();++i){
                                        if( i != 0 )
                                                ostr << "-+-";
                                        ostr << std::string(widths[i],'-');
                                }
                        } else{
                                for(size_t i=0;i!=line.size();++i){
                                        if( i != 0 )
                                                ostr << " | ";
                                        ostr << pad(line[i], widths[i]);
                                }
                        }
                        ostr << "\n";
                }
        }
} // end namespace ps
#endif // PS_APP_PRETTY_PRINTER_H
