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


        template<class Primitive_Type>
        struct basic_equity_breakdown{
                using prim_t = Primitive_Type;
                struct player_t{
                        virtual double equity()const=0;
                        virtual prim_t nwin(size_t idx)const=0;
                        virtual prim_t sigma()const=0;
                        virtual prim_t win()const{  return nwin(0); }
                        virtual prim_t draw()const{ return nwin(1); }
                        virtual prim_t lose()const=0;
                };
                virtual prim_t sigma()const=0;
                virtual size_t n()const=0;
                virtual player_t const& player(size_t idx)const=0;
        };
                
        template<class Primitive_Type>
        struct basic_equity_breakdown_permutation_view : basic_equity_breakdown<Primitive_Type>{
                basic_equity_breakdown_permutation_view(std::shared_ptr<basic_equity_breakdown<Primitive_Type> > const impl, std::vector<int> perm)
                        : impl_{impl}
                        , perm_{perm}
                {
                        //PRINT( detail::to_string(perm_));
                }
                size_t sigma()const override{ return impl_->sigma(); }
                size_t n()    const override{ return impl_->n(); }
                typename basic_equity_breakdown<Primitive_Type>::player_t const& player(size_t idx)const override{
                        return impl_->player( perm_[idx] );
                }
        private:
                std::shared_ptr<basic_equity_breakdown<Primitive_Type> const> impl_;
                std::vector<int> perm_;
        }; 


        using equity_breakdown                  = basic_equity_breakdown<size_t>;
        using equity_breakdown_player           = basic_equity_breakdown<size_t>::player_t;
        using equity_breakdown_permutation_view = basic_equity_breakdown_permutation_view<size_t>;


                
        std::ostream& operator<<(std::ostream& ostr, equity_breakdown const& self);

} // ps
        

#endif // PS_EVAL_EQUITY_BREAKDOWN_H
