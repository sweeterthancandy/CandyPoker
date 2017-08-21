#include "ps/eval/rank_world.h"
#include "ps/base/generate.h"

namespace ps{
namespace {
        struct rank_world_aux{
                rank_world_aux(){
                        result.emplace_back(); // dummy one because we start at 1
                }
                void next( bool f, rank_id a, rank_id b, rank_id c, rank_id d, rank_id e){
                        rank_vector aux{a,b,c,d,e};
                        result.emplace_back(order_, f, name_proto_, std::move(aux));
                        ++order_;
                }
                void begin(std::string const& name){
                        name_proto_ = name;
                }
                void end(){}
                ranking_t order_ = 1;
                std::string name_proto_;
                std::vector<ranking_decl> result;
        };
} // anon
                
rank_world::rank_world(){
        rank_world_aux aux;
        generate(aux);
        world_ = std::move(aux.result);
}

namespace {
        int reg = ( rank_word_factory::register_<rank_world>(), 0 );
} // anon

} // ps
