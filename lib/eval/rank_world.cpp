#include "ps/eval/rank_decl.h"
#include "ps/base/visit_poker_rankings.h"

namespace ps{
namespace {
        struct rank_world_aux{
                rank_world_aux(){
                        result.emplace_back(); // dummy one because we start at 1
                }
                void next( bool f, rank_id a, rank_id b, rank_id c, rank_id d, rank_id e){
                        rank_vector aux{a,b,c,d,e};
                        result.emplace_back(order_, cat_, f, name_proto_, std::move(aux));
                        ++order_;
                }
                void begin(std::string const& name){
                        static std::map<std::string, hand_rank_category> aux = {
                                {"Royal Flush"        , HR_RoyalFlush},
                                {"Straight Flush"     , HR_StraightFlush},
                                {"Quads"              , HR_Quads},
                                {"Full House"         , HR_FullHouse},
                                {"Flush"              , HR_Flush},
                                {"Straight"           , HR_Straight},
                                {"Trips"              , HR_Trips},
                                {"Two pair"           , HR_TwoPair},
                                {"One pair"           , HR_OnePair},
                                {"High Card"          , HR_HighCard},
                        };
                        name_proto_ = name;
                        cat_ = aux.find(name)->second;
                }
                void end(){}
                ranking_t order_ = 1;
                hand_rank_category cat_{HR_NotAHandRank};
                std::string name_proto_;
                std::vector<ranking_decl> result;
        };
} // anon
                
rank_world::rank_world(){
        rank_world_aux aux;
        visit_poker_rankings(aux);
        world_ = std::move(aux.result);
}

namespace {
        int reg = ( rank_word_factory::register_<rank_world>(), 0 );
} // anon

} // ps
