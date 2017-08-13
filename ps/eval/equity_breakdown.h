#ifndef PS_EVAL_EQUITY_BREAKDOWN_H
#define PS_EVAL_EQUITY_BREAKDOWN_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <memory>

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


        struct equity_breakdown_permutation_view : equity_breakdown{
                equity_breakdown_permutation_view(std::shared_ptr<equity_breakdown const> impl, std::vector<int> perm)
                        : impl_{impl}
                        , perm_{perm}
                {
                        //PRINT( detail::to_string(perm_));
                }
                size_t sigma()const override{ return impl_->sigma(); }
                size_t n()    const override{ return impl_->n(); }
                equity_breakdown_player const& player(size_t idx)const override{
                        return impl_->player( perm_[idx] );
                }
        private:
                std::shared_ptr<equity_breakdown const> impl_;
                std::vector<int> perm_;
        }; 
        

                
        std::ostream& operator<<(std::ostream& ostr, equity_breakdown const& self);

} // ps
        

#endif // PS_EVAL_EQUITY_BREAKDOWN_H
