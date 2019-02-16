#include "lib/eval/dispatch_table.h"
#include "lib/eval/generic_shed.h"
#include "lib/eval/generic_sub_eval.h"
#include "lib/eval/optimized_transform.h"

namespace ps{

struct dispatch_generic : dispatch_table{
        using transform_type =
                optimized_transform<
                        generic_sub_eval,
                        generic_shed,
                        basic_sub_eval_factory,
                        rank_hash_eval>;


        virtual bool match(dispatch_context const& dispatch_ctx)const override{
                return true;
        }
        virtual std::shared_ptr<optimized_transform_base> make()const override{
                return std::make_shared<transform_type>();
        }
        virtual std::string name()const override{
                return "generic";
        }
};
static register_disptach_table<dispatch_generic> reg_dispatch_generic;


} // end namespace ps
