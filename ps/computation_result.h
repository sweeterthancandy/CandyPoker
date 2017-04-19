#ifndef PS_COMPUTATION_RESULT_H
#define PS_COMPUTATION_RESULT_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "ps/frontend.h"

namespace ps{
        namespace numeric{
        
                namespace bnu = boost::numeric::ublas;
                
                struct result_type{

                        enum Tags{
                                NTag_Wins,
                                NTag_Draws,
                                NTag_Sigma,

                                RTag_Equity = 0
                        };

                        explicit result_type(size_t n = 9):
                                nat_mat(n,4,0),
                                rel_mat(n,1,0.0)
                        {}
                        friend std::ostream& operator<<(std::ostream& ostr, result_type const& self){
                                return ostr << "nat_mat = " << self.nat_mat << ", real = " << self.rel_mat;
                        }

                        result_type& operator+=(result_type const& that){
                                nat_mat  += that.nat_mat;
                                rel_mat += that.rel_mat;
                                return *this;
                        }

                        template<class Parent_Type>
                        struct player_view{
                                explicit player_view(Parent_Type* parent, size_t idx)
                                        :parent_{parent},idx_{idx}
                                {}
                                auto wins()  { return parent_->nat_mat(idx_, NTag_Wins); }
                                auto draws() { return parent_->nat_mat(idx_, NTag_Draws); }
                                auto sigma() { return parent_->nat_mat(idx_, NTag_Sigma); }
                                auto equity(){ return parent_->rel_mat(idx_, NTag_Wins) / sigma(); }
                        private:
                                Parent_Type* parent_;
                                size_t idx_;
                        };


                        auto operator()(size_t idx){
                                return player_view<result_type      >{this,idx};
                        }
                        auto operator()(size_t idx)const{
                                return player_view<result_type const>{this,idx};
                        }


                        using nat_matrix_type = bnu::matrix<std::uint64_t>;
                        using real_matrix_type = bnu::matrix<long double>;
                        
                        bnu::matrix<std::uint64_t> nat_mat;
                        bnu::matrix<long double>   rel_mat;
                };
        }
}

#endif // PS_COMPUTATION_RESULT_H
