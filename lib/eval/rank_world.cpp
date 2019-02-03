/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
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
