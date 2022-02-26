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
#ifndef PS_INTERFACE_H
#define PS_INTERFACE_H


#include <sstream>
#include <mutex>
#include <unordered_map>

#include <boost/range/algorithm.hpp>
#include <boost/timer/timer.hpp>
#include <boost/rational.hpp>
#include "ps/eval/rank_decl.h"
#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

#include "ps/base/visit_poker_rankings.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/prime_rank_map.h"

/*
Need this for unit testing, as really the evaluation is configurable in the application, but
I also want this in the intergration tests
*/
namespace ps {
namespace interface_ {

/*
The result from computation is a matrix with the interpretation
	m(i,j) => number of jth way draws for player i,

	for three player
	    | p1_wins   p1_two_way_draws   three_way_draws |
	m = | p2_wins   p2_two_way_draws   three_way_draws |
	    | p3_wins   p3_two_way_draws   three_way_draws |

*/

using rational_ty = boost::rational<unsigned long long>;

class EvaulationResultView{
public:
	using vec_ty = std::vector<unsigned long long>;
	class PlayerView
	{
	public:
		PlayerView(
			std::string const& range,
			const rational_ty& equity)
			: range_{ range }, equity_{ equity }
		{}
		const auto& Range()const { return range_; }
		auto Wins()const{ return }
		const auto& EquityAsRational()const { return equity_; }
		const double EquityAsDouble()const { return boost::rational_cast<double>(equity_); }
	private:
		std::string range_;
		rational_ty equity_;
		unsigned long long wins_;
		unsigned long long ties_;
	};

	EvaulationResultView(
		std::vector<std::string>& player_ranges,
		const matrix_t& result)
		: player_ranges_{player_ranges}
		, result_{ result }
	{
		if (result.rows() != result.cols())
		{
			BOOST_THROW_EXCEPTION(std::domain_error("matrix not square"));
		}
		if (result.rows() != player_ranges.size())
		{
			BOOST_THROW_EXCEPTION(std::domain_error("does not look like result matrix"));
		}

		const auto num_players = player_ranges.size();
		// need to cache row sums and totals
		std::unordered_map<size_t, rational_ty> sigma_device;
		for (size_t i = 0; i != num_players; ++i) {
			for (size_t j = 0; j != num_players; ++j) {
				sigma_device[j] += result(j, i);
			}
		}
		rational_ty sigma = 0;
		for (size_t i = 0; i != num_players; ++i) {
			sigma += sigma_device[i] / rational_ty(i + 1);
		}

		for (size_t i = 0; i != num_players; ++i)
		{
			rational_ty equity{ 0 };
			for (size_t j = 0; j != num_players; ++j) {
				equity += result(j, i) / rational_ty(j + 1);
			}
			equity /= sigma;
			players_.emplace_back(player_ranges[i], equity);
		}

	}
	const auto& players()const { return players_; }
	auto begin_players()const { return players_.end(); }
	auto end_players()const { return players_.end(); }
	const auto& player_view(size_t idx)const {
		return players_[idx];
	}
private:
	std::vector<std::string> player_ranges_;
	matrix_t result_;
	std::vector< PlayerView> players_;
};

EvaulationResultView evaluate(std::vector<std::string>& player_ranges, std::string const& engine = {});

} // end namespace interface_
} // end namespace ps

#endif // PS_INTERFACE_H
