#ifndef EQUITY_EVAL_RESULT_H
#define EQUITY_EVAL_RESULT_H

#include <vector>
#include <ostream>
#include <numeric>

#include "ps/eval/player_view.h"
#include "ps/support/array_view.h"

namespace ps{

struct equity_eval_result{

        // only if all convertiable to size_t
        equity_eval_result(size_t n):
                n_{n},
                data_(n*n)
        {
        }
        size_t const& data_access(size_t i, size_t j)const{
                return data_.at(i * n_ + j);
                //return data_[i * n_ + j];
                //PRINT_SEQ((n_)(i)(j)(i * n_ + j)(data_.size()));
        }
        size_t& data_access(size_t i, size_t j){
                return const_cast<size_t&>(
                        reinterpret_cast<equity_eval_result const*>(this)
                                ->data_access(i,j)
                );
        }
        size_t const* data()const{ return reinterpret_cast<size_t const*>(&data_.front()); }

        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & sigma_;
                ar & data_;
        }

        auto sigma()const{ return sigma_; }
        auto& sigma(){ return sigma_; }

        auto player(size_t idx)const{
                return player_view_t(n_,
                                     sigma_,
                                     support::array_view<size_t>(&data_[0] + idx * n_, n_ ));
        }

        auto n()const{ return n_; }
        
        friend std::ostream& operator<<(std::ostream& ostr, equity_eval_result const& self){
                PRINT(detail::to_string(self.data_));
                std::vector<std::vector<std::string> > line_buffer;
                std::vector<size_t> widths(self.n_, 0);
                ostr << self.sigma() << "\n";
                for(size_t i=0;i!=self.n_;++i){
                        line_buffer.emplace_back();
                        for(size_t j=0;j!=self.n_;++j){
                                line_buffer.back().emplace_back(
                                        boost::lexical_cast<std::string>(
                                                self.player(i).nwin(j)));
                                widths[j] = std::max(widths[j], line_buffer.back().back().size());
                        }
                }
                for(size_t i=0;i!=self.n_;++i){
                        for(size_t j=0;j!=self.n_;++j){
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
        
private:
        size_t sigma_ = 0;
        size_t n_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::vector<size_t> data_;
};

} // ps

#endif // EQUITY_EVAL_RESULT_H
