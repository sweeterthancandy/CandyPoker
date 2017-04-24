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
                }
        private:
                calculation_context* ctx_;
                std::set< symbolic_primitive* > prims_;
        };
} // transform
} // ps

#endif // PS_TRANSFORM_CALC_PRIMITIVE_H
