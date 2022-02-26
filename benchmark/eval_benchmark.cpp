#include <benchmark/benchmark.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"

using namespace ps;
using namespace ps::interface_;
static void BM_TwoPlayerPocketPairGenericFirst(benchmark::State& state) {
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
