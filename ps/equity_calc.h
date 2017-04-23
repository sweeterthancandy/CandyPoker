#ifndef PS_EQUITY_CALC_H
#define PS_EQUITY_CALC_H

#include <vector>

#include "ps/eval.h"
#include "ps/detail/visit_combinations.h"
#include "ps/cards.h"
#include "ps/frontend.h"

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace bnu = boost::numeric::ublas;

namespace ps{

        struct equity_calc{

                bool run( bnu::matrix<size_t>& result,
                          std::vector<holdem_id> const& players,
                          std::vector<card_id> const& board = std::vector<card_id>{},
                          std::vector<card_id> const& dead = std::vector<card_id>{})noexcept;
        private:
                template<size_t Num_Players>
                bool run_p( bnu::matrix<size_t>& result,
                            std::vector<holdem_id> const& players,
                            std::vector<card_id> const& board,
                            std::vector<card_id> const& dead)noexcept;
                template<size_t Num_Players, size_t Num_Deal>
                bool run_pd( bnu::matrix<size_t>& result,
                             std::vector<holdem_id> const& players,
                             std::vector<card_id> const& board,
                             std::vector<card_id> const& dead)noexcept;
                eval eval_;
        };

}

#endif // PS_EQUITY_CALC_H
