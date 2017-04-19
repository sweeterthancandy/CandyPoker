#ifndef PS_COMPUTATION_RESULT_H
#define PS_COMPUTATION_RESULT_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>

#include <boost/format.hpp>

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
                                n_{n},
                                nat_mat(n_,4,0),
                                rel_mat(n_,1,0.0)
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
                                auto wins()  const{ return parent_->nat_mat(idx_, NTag_Wins); }
                                auto draws() const{ return parent_->nat_mat(idx_, NTag_Draws); }
                                auto sigma() const{ return parent_->nat_mat(idx_, NTag_Sigma); }
                                auto equity()const{ return parent_->rel_mat(idx_, RTag_Equity) / sigma(); }
                                friend std::ostream& operator<<(std::ostream& ostr, player_view const& self){
                                        return  ostr 
                                                << boost::format("wins=%-10d draws=%-10d sigma=%-10d equity=%-2.4f")
                                                % self.wins() % self.draws() % self.sigma() % self.equity();
                                }
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


                        auto proxy()const{
                                std::vector<player_view<result_type const> > aux;
                                for(size_t i=0;i!=n_;++i)
                                        aux.emplace_back(this,i);
                                return std::move(aux);
                        }
                        


                        using nat_matrix_type = bnu::matrix<std::uint64_t>;
                        using real_matrix_type = bnu::matrix<long double>;
                        
                        size_t n_;
                        bnu::matrix<std::uint64_t> nat_mat;
                        bnu::matrix<long double>   rel_mat;

                };
        }
}

#endif // PS_COMPUTATION_RESULT_H
