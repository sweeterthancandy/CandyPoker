#ifndef PS_EVAL_CLASS_EQUITY_FUTURE_H
#define PS_EVAL_CLASS_EQUITY_FUTURE_H

namespace ps{

struct class_equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        class_equity_future()
        {
        }
        result_t schedual_group(support::processor::process_group& pg, holdem_class_vector const& players){
                
                std::vector< std::tuple< std::vector<int>, result_t > > items;

                for( auto hv : players.get_hand_vectors()){
                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);
                        auto fut = ef_.schedual_group(pg, perm_players);
                        items.emplace_back(perm, fut);
                }
                pg.sequence_point();
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [n_=players.size(),items_=std::move(items),this](){
                        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(n_);
                        for( auto& t : items_ ){
                                result->append(
                                        *std::make_shared<equity_breakdown_permutation_view>(
                                                std::get<1>(t).get(),
                                                std::get<0>(t)));
                        }
                        return result;
                });
                result_t fut = task->get_future();
                m_.emplace(players, fut);
                pg.push([task]()mutable{ (*task)(); });
                return fut;
        }
private:
        mutable equity_future ef_;
        std::map< holdem_hand_vector, result_t > m_;
};

} // ps

#endif // PS_EVAL_CLASS_EQUITY_FUTURE_H
