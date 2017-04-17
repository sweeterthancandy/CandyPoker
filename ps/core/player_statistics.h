#ifndef PS_PLAYER_STATISTICS_H
#define PS_PLAYER_STATISTICS_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace ps{
        
        namespace bnu = boost::numeric::ublas;
        
        enum{
                stat_wins,
                stat_draws,
                stat_loses,
                stat_sigma,
                sizeof_stat
        };

        struct statistics_memory{
                statistics_memory(size_t players):
                        mat_(players, sizeof_stat, 0)
                {}

                size_t num_players_;
                bnu::matrix<int> mat_;
        };

        struct player_statistics_view{

                player_statistics_view(statistics_memory& stat, size_t idx)
                        :stat_{&stat}
                        ,idx_{idx}
                {}
                int get_wins(){ return stat_->mat_(idx_, stat_wins); }
                int get_draws(){ return stat_->mat_(idx_, stat_draws); }
                int get_sigma(){ return stat_->mat_(idx_, stat_sigma); }

        private:
                statistics_memory* stat_;
                size_t idx_;
        };
}

#endif // PS_PLAYER_STATISTICS_H
