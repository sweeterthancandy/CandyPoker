#ifndef PS_EVAL_EQUITY_CALCULATOR_IMPL_H
#define PS_EVAL_EQUITY_CALCULATOR_IMPL_H

#include "ps/eval/equity_evaulator.h"

struct equity_evaulator_impl : public equity_evaluator{
        equity_eval_result evaluate(std::vector<holdem_id> const& players)const override{
                // we first need to enumerate every run of the board,
                // for this we can create a mapping [0,51-n*2] -> [0,51],
                equity_eval_result result{2};

                // vector of first and second card
                std::vector<card_id> x,y;
                std::vector<card_id> known;

                for( auto const& p : players){
                        x.push_back(holdem_hand_decl::get(p).first().id());
                        y.push_back(holdem_hand_decl::get(p).second().id());
                }
                auto n = players.size();

                boost::copy( x, std::back_inserter(known));
                boost::copy( y, std::back_inserter(known));

                auto const& eval = evaluator_factory::get("5_card_map");
        
                for(board_combination_iterator iter(5, known),end;iter!=end;++iter){

                        auto const& b(*iter);

                        std::vector<ranking_t> ranked;
                        for( int i=0;i!=n;++i){
                                ranked.push_back(eval.rank(x[i], y[i],
                                                            b[0], b[1], b[2], b[3], b[4]) );
                        }
                        auto lowest = ranked[0] ;
                        size_t count{1};
                        for(size_t i=1;i<ranked.size();++i){
                                if( ranked[i] == lowest ){
                                        ++count;
                                } else if( ranked[i] < lowest ){
                                        lowest = ranked[i]; 
                                        count = 1;
                                }
                        }
                        for(size_t i=0;i!=ranked.size();++i){
                                if( ranked[i] == lowest ){
                                        ++result.data_access(i,count-1);
                                }
                        }
                        ++result.sigma();
                }

                return std::move(result);
        }
};

#endif // PS_EVAL_EQUITY_CALCULATOR_IMPL_H
