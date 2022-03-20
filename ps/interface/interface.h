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

#include "ps/base/cards_fwd.h"

#include <string>
#include <numeric>
#include <vector>
#include <unordered_map>

// boost rational stuff
#pragma warning( disable : 4146 )
#include <boost/rational.hpp>
#include <boost/optional.hpp>

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

// TODO, handle issues with very large numbers, ie std::optional<boost::rational<rational_int_ty>> where an issue
// comes up
using rational_int_ty = std::uintmax_t;
using rational_ty = boost::rational<rational_int_ty>;

class EvaulationResultView{
public:
	using vec_ty = std::vector<unsigned long long>;
	class PlayerView
	{
	public:
		PlayerView(
			std::string const& range,
			unsigned long long sigma,
			const rational_ty& equity,
			std::vector<unsigned long long> const& win_vec)
			: range_{ range }, sigma_{sigma}, equity_{ equity }, win_vec_{win_vec}
		{}
		const auto& Range()const { return range_; }
		auto Wins()const {
			return win_vec_.at(0);
		}
		auto NWins(size_t draw_index)const {
			return win_vec_.at(draw_index);
		}
		auto AnyDraws()const {
			return std::accumulate(
				win_vec_.begin()+1,
				win_vec_.end(),
				static_cast<unsigned long long>(0));
		}
		double AnyDrawsNormalised()const
		{
			rational_ty rat = 0;
			for (size_t draw_index = 1; draw_index != win_vec_.size(); ++draw_index)
			{
				rat += rational_ty{ win_vec_[draw_index]} / (draw_index + 1);
			}
			return boost::rational_cast<double>(rat);
		}
		
		const auto& EquityAsRational()const { return equity_; }
		const double EquityAsDouble()const { return boost::rational_cast<double>(equity_); }

		auto const& NWinVector()const{ return win_vec_; }

		unsigned long long Sigma()const{ return sigma_; }
	private:
		std::string range_;
		unsigned long long sigma_;
		rational_ty equity_;
		std::vector<unsigned long long>  win_vec_;
	};

	EvaulationResultView(
		const std::vector<std::string>& player_ranges,
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
		// draw index -> total count 
		std::unordered_map<size_t, rational_ty> sum_per_draw_index;
		for (size_t draw_index = 0; draw_index != num_players; ++draw_index) {
			for (size_t player_index = 0; player_index != num_players; ++player_index) {
				sum_per_draw_index[draw_index] += result(player_index, draw_index);
			}
		}
		rational_ty sigma = 0;
		for (size_t draw_index = 0; draw_index != num_players; ++draw_index) {
			// on the outright win index, no double counting
			// on the 2-player draw index, double counting
			// on the 3-player draw index, tripped counting
			// ...
			sigma += sum_per_draw_index[draw_index] / rational_ty(draw_index + 1);
		}

		PS_ASSERT(sigma.denominator() == 1, "unexpected");

		for (size_t player_index = 0; player_index != num_players; ++player_index)
		{
			rational_ty equity{ 0 };
			std::vector<unsigned long long> win_vec;
			for (size_t draw_index = 0; draw_index != num_players; ++draw_index) {
				equity += static_cast<rational_int_ty>(result(player_index, draw_index)) / rational_ty(draw_index + 1);
				win_vec.push_back(result(player_index, draw_index));
			}

			// skip 0/0 issues
			if( sigma != 0)
				equity /= sigma;
			players_.emplace_back(
				player_ranges[player_index],
				static_cast<unsigned long long>(sigma.numerator()),
				equity,
				std::move(win_vec));
		}

	}
	const auto& players()const { return players_; }
	const auto& player_view(size_t idx)const {
		return players_[idx];
	}

	matrix_t const& get_matrix()const { return result_; }
private:
	std::vector<std::string> player_ranges_;
	matrix_t result_;
	std::vector< PlayerView> players_;
};

class EvaluationObjectImpl;
class EvaluationObject
{
public:
	enum FlagsMask : unsigned
	{
		F_None = 0,
		F_DebugInstructions = 1 << 1,
		F_ShowInstructions = 1 << 2,
		F_StepPercent = 1 << 3,
		F_TimeInstructionManager = 1 << 4,
		F_WriteCsv = 1 << 5,
		F_CacheInstructions = 1 << 6,

		F_Defaults = F_None,

	};
	using Flags = unsigned;

	EvaluationObject(std::shared_ptr< EvaluationObjectImpl> impl) :impl_{ impl } {}
	EvaluationObject(std::vector<std::string> const& player_ranges, std::string const& engine = {}, Flags flags = F_Defaults)
		: EvaluationObject{ std::vector<std::vector<std::string> >{player_ranges} , engine , flags }
	{}
	EvaluationObject(std::vector<std::vector<std::string> > const& player_ranges_list, boost::optional<std::string> const& engine = {}, Flags flags = F_Defaults);

	EvaulationResultView Compute()const
	{
		auto const result = this->ComputeList();
		PS_ASSERT(result.size() == 1, "not a single computation");
		return result.at(0);
	}
	
	std::vector<EvaulationResultView> ComputeList()const;
	boost::optional<EvaluationObject> Prepare()const;

	static void BuildCache();
private:
	std::shared_ptr< EvaluationObjectImpl> impl_;
};

inline void test_prepare(std::vector<std::string> const& player_ranges, std::string const& engine = {})
{
	EvaluationObject obj(player_ranges, engine);
	obj.Prepare();
}


inline EvaulationResultView evaluate(std::vector<std::string> const& player_ranges, std::string const& engine = {})
{
	EvaluationObject obj(player_ranges, engine);
	return obj.Compute();
}

inline std::vector<EvaulationResultView> evaluate_list(std::vector<std::vector<std::string> > const& player_ranges_list, std::string const& engine = {})
{
	EvaluationObject obj(player_ranges_list, engine);
	return obj.ComputeList();
}


} // end namespace interface_
} // end namespace ps

#endif // PS_INTERFACE_H
