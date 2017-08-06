#ifndef PS_EVAL_EQUITY_BREAKDOWN_H
#define PS_EVAL_EQUITY_BREAKDOWN_H

#include <vector>
#include <boost/lexical_cast.hpp>

namespace ps{

        // virtual interface to view results from 
        // equity calculations
        //
        // Need virtual interface, because sometimes want to
        // return a permutation of other results


        struct equity_breakdown_player{
                virtual double equity()const=0;
                virtual size_t nwin(size_t idx)const=0;
                virtual size_t sigma()const=0;
                virtual size_t win()const{  return nwin(0); }
                virtual size_t draw()const{ return nwin(1); }
                virtual size_t lose()const=0;
        };

        struct equity_breakdown{
                virtual size_t sigma()const=0;
                virtual size_t n()const=0;
                virtual equity_breakdown_player const& player(size_t idx)const=0;

        };
                
        inline std::ostream& operator<<(std::ostream& ostr, equity_breakdown const& self){
                std::vector<std::vector<std::string> > line_buffer;
                std::vector<size_t> widths(self.n(), 0);
                ostr << self.sigma() << "\n";
                for(size_t i=0;i!=self.n();++i){
                        line_buffer.emplace_back();
                        for(size_t j=0;j!=self.n();++j){
                                line_buffer.back().emplace_back(
                                        boost::lexical_cast<std::string>(
                                                self.player(i).nwin(j)));
                                widths[j] = std::max(widths[j], line_buffer.back().back().size());
                        }
                }
                for(size_t i=0;i!=self.n();++i){
                        for(size_t j=0;j!=self.n();++j){
                                auto const& tok(line_buffer[i][j]);
                                size_t padding{widths[j]-tok.size()};
                                size_t left_pad{padding/2};
                                size_t right_pad{padding - left_pad};
                                if( j != 0 ){
                                        ostr << " | ";
                                }
                                if( left_pad )
                                        ostr << std::string(left_pad,' ');
                                ostr << tok;
                                if( right_pad )
                                        ostr << std::string(right_pad,' ');

                        }
                        ostr << "\n";
                }
                return ostr;
        }

} // ps

#endif // PS_EVAL_EQUITY_BREAKDOWN_H
