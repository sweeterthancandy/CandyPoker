#ifndef PS_SIM_HOLDEM_CLASS_STRATEGY_H
#define PS_SIM_HOLDEM_CLASS_STRATEGY_H

#include "ps/base/cards.h"

namespace ps{

struct holdem_class_strategy{
        holdem_class_strategy(double fill = 1.0){
                for(size_t i{0};i!=169;++i){
                        vec_[i] = fill;
                }
        }
        auto begin()const{ return vec_.begin(); }
        auto end()  const{ return vec_.end();   }
        double&       operator[](size_t idx)     {return vec_[idx]; }
        double const& operator[](size_t idx)const{return vec_[idx]; }
        void check(){
                for(size_t i{0};i!=169;++i){
                        if ( !( 0 <= vec_[i] && vec_[i] <= 1.0 && "not a strat") ){
                                std::cerr << "FAILED\n";
                                return;
                        }
                }
        }
        holdem_class_strategy& operator*=(double val){
                for(size_t i{0};i!=169;++i){
                        vec_[i] *= val;
                }
                return *this;
        }
        holdem_class_strategy& operator+=(holdem_class_strategy const& that){
                for(size_t i{0};i!=169;++i){
                        vec_[i] += that.vec_[i];
                }
                return *this;
        }
        holdem_class_strategy& operator-=(holdem_class_strategy const& that){
                for(size_t i{0};i!=169;++i){
                        vec_[i] -= that.vec_[i];
                }
                return *this;
        }

        holdem_class_strategy operator*(double val){
                holdem_class_strategy result{*this};
                result *= val;
                return std::move(result);
        }
        holdem_class_strategy operator+(holdem_class_strategy const& that){
                holdem_class_strategy result{*this};
                result += that;
                return std::move(result);
        }
        holdem_class_strategy operator-(holdem_class_strategy const& that){
                holdem_class_strategy result{*this};
                result -= that;
                return std::move(result);
        }
        double sigma(){
                return std::accumulate(vec_.begin(), vec_.end(), 0.0);
        }

        double norm()const{
                double result{0.0};
                for(size_t i{0};i!=169;++i){
                        result = std::max(result, std::fabs(vec_[i]));
                }
                return result;
        }
        friend std::ostream& operator<<(std::ostream& ostr, holdem_class_strategy const& strat){
                return ostr << ps::detail::to_string(strat.vec_);
        }
        // print pretty table
        //
        //      AA  AKs ... A2s
        //      AKo KK
        //      ...     ...
        //      A2o         22
        //
        //
        void display(){
                /*
                        token_buffer[0][0] token_buffer[1][0]
                        token_buffer[0][1]

                        token_buffer[y][x]


                 */
                std::array<
                        std::array<std::string, 13>, // x
                        13                           // y
                > token_buffer;
                std::array<size_t, 13> widths;

                for(size_t i{0};i!=169;++i){
                        auto const& decl =  holdem_class_decl::get(i) ;
                        size_t x{decl.first().id()};
                        size_t y{decl.second().id()};
                        // inverse
                        x = 12 - x;
                        y = 12 - y;
                        if( decl.category() == holdem_class_type::offsuit ){
                                std::swap(x,y);
                        }

                        #if 1
                        //token_buffer[y][x] = boost::lexical_cast<std::string>(vec_[i]);
                        if( vec_[i] == 1.0 ){
                                token_buffer[y][x] = "1";
                        } else if( vec_[i] == 0.0 ){
                                token_buffer[y][x] = "0";
                        } else {
                                token_buffer[y][x] = str(boost::format("%.4f") % vec_[i]);
                        }

                        #else
                        token_buffer[y][x] = boost::lexical_cast<std::string>(decl.to_string());
                        #endif
                }
                for(size_t i{0};i!=13;++i){
                        widths[i] = std::max_element( token_buffer[i].begin(),
                                                      token_buffer[i].end(),
                                                      [](auto const& l, auto const& r){
                                                              return l.size() < r.size(); 
                                                      })->size();
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
                
                std::cout << "   ";
                for(size_t i{0};i!=13;++i){
                        std::cout << pad( rank_decl::get(12-i).to_string(), widths[i] ) << " ";
                }
                std::cout << "\n";
                std::cout << "  +" << std::string( std::accumulate(widths.begin(), widths.end(), 0) + 13, '-') << "\n";

                for(size_t i{0};i!=13;++i){
                        std::cout << rank_decl::get(12-i).to_string() << " |";
                        for(size_t j{0};j!=13;++j){
                                if( j != 0){
                                        std::cout << " ";
                                }
                                std::cout << pad(token_buffer[j][i], widths[j]);
                        }
                        std::cout << "\n";
                }
        }
        void transform(std::function<double(size_t i, double)> const& t){
                for(size_t i{0};i!=169;++i){
                        vec_[i] = t(i, vec_[i]);
                }
        }
        bool operator<(holdem_class_strategy const& that)const{
                return vec_ < that.vec_;
        }
private:
        std::array<double, 169> vec_;
};

} // ps

#endif // PS_SIM_HOLDEM_CLASS_STRATEGY_H

