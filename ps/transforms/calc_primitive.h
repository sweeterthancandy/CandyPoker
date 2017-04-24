#ifndef PS_TRANSFORM_CALC_PRIMITIVE_H
#define PS_TRANSFORM_CALC_PRIMITIVE_H

#include "ps/transforms/transform.h"
#include "ps/detail/work_schedular.h"

namespace ps{
namespace transforms{

        struct calc_primitive : symbolic_transform{
                explicit calc_primitive(calculation_context& ctx):symbolic_transform{"calc_primitive"},ctx_{&ctx}{}
                bool apply(symbolic_computation::handle& ptr)override{
                        if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Primitive )
                                return false;

                        prims_.insert( reinterpret_cast<symbolic_primitive*>(ptr.get()) );
                        return true;
                }
                void end()override{
                        detail::work_scheduler sch;
                        for( auto ptr : prims_){
                                auto j = [this,ptr](){
                                        ptr->calculate( *ctx_ );
                                };
                                sch.decl( std::move(j) );
                        }
                        sch.run();

                        // pretty print
                        for( auto ptr : prims_){
                                auto ret{ ptr->calculate(*ctx_)  };
                                std::cout << ptr->to_string() << " | ";
                                for(size_t i=0; i != ptr->get_hands().size(); ++i){
                                        auto eq{
                                                static_cast<double>(ret(i,10) ) / computation_equity_fixed_prec / ret(i,9) * 100 };
                                        std::cout << boost::format("%8.4f | ") % eq;
                                }
                                std::cout << "\n";
                        }
                }
        private:
                calculation_context* ctx_;
                std::set< symbolic_primitive* > prims_;
        };

} // transform
} // ps

#endif // PS_TRANSFORM_CALC_PRIMITIVE_H
