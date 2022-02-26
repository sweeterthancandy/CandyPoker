#include <benchmark/benchmark.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

using namespace ps;
using namespace ps::interface_;
static void BM_TwoPlayerPocketPairGenericFirst(benchmark::State& state) {

    namespace logging = boost::log;

    logging::trivial::severity_level my_log_level = logging::trivial::error;
    auto filt = logging::filter(logging::trivial::severity >= my_log_level);
    logging::core::get()->set_filter(filt);

    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        evaluate(player_ranges);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_TwoPlayerPocketPairGenericFirst);

static void BM_TwoPlayerPocketPairGeneric(benchmark::State& state) {
  for (auto _ : state)
  {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        evaluate(player_ranges);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_TwoPlayerPocketPairGeneric);

static void BM_ThreePlayerPocketPairGeneric(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ"};
        evaluate(player_ranges);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_ThreePlayerPocketPairGeneric);


BENCHMARK_MAIN();
