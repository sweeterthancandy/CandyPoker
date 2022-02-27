#include <benchmark/benchmark.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"
#include "ps/eval/instruction.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

using namespace ps;
using namespace ps::interface_;

inline auto do_frontend_parse(std::string const& s)
{
    return frontend::parse(s);
}


static void BM_FrontPlayerRangeSimple(benchmark::State& state) {

    for (auto _ : state)
    {
        do_frontend_parse("AA");
    }
}
static void BM_FrontPlayerRangeComplex(benchmark::State& state) {

    for (auto _ : state)
    {
        do_frontend_parse("45s+,68s+,22+,ATo+,AJs+");
    }
}
BENCHMARK(BM_FrontPlayerRangeSimple);
BENCHMARK(BM_FrontPlayerRangeComplex);



static void BM_FrontTwoPlayerTreeSimple(benchmark::State& state) {
    
    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        tree_range root({ p0,p1 });
    }
}

static void BM_FrontThreePlayerTreeSimple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        tree_range root({ p0,p1,p2 });
    }
}

static void BM_FrontFourPlayerTreeSimple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        auto p3 = do_frontend_parse("JJ");
        tree_range root({ p0,p1,p2,p3 });
    }
}
BENCHMARK(BM_FrontTwoPlayerTreeSimple);
BENCHMARK(BM_FrontThreePlayerTreeSimple);
BENCHMARK(BM_FrontFourPlayerTreeSimple);







static void BM_TwoPlayerInstructionBuilderSimple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        frontend_to_instruction_list("dummy", { p0, p1 });
    }
}
static void BM_ThreePlayerInstructionBuilderSimple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        frontend_to_instruction_list("dummy", { p0, p1, p2 });
    }
}
static void BM_FourPlayerInstructionBuilderSimple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        auto p3 = do_frontend_parse("JJ");
        frontend_to_instruction_list("dummy", { p0, p1, p2, p3 });
    }
}
BENCHMARK(BM_TwoPlayerInstructionBuilderSimple);
BENCHMARK(BM_ThreePlayerInstructionBuilderSimple);
BENCHMARK(BM_FourPlayerInstructionBuilderSimple);







static void BM_TwoPlayerPocketPairGenericFirst(benchmark::State& state) {

    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        evaluate(player_ranges);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_TwoPlayerPocketPairGenericFirst);






static void BM_TwoPlayerPocketPairGenericPrepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        test_prepare(player_ranges);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_TwoPlayerPocketPairGenericPrepare);

static void BM_ThreePlayerPocketPairGenericPrepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        test_prepare(player_ranges);
    }
}
// Register the function as a benchmark
BENCHMARK(BM_ThreePlayerPocketPairGenericPrepare);



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

static void BM_ThreePlayerPocketPairTPG(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        evaluate(player_ranges, "three-player-generic");
    }
}
// Register the function as a benchmark
BENCHMARK(BM_ThreePlayerPocketPairTPG);

static void BM_ThreePlayerPocketPairTPP(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        evaluate(player_ranges, "three-player-perm");
    }
}
// Register the function as a benchmark
BENCHMARK(BM_ThreePlayerPocketPairTPP);

static void BM_ThreePlayerPocketPairAVX2(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        evaluate(player_ranges, "three-player-avx2");
    }
}
// Register the function as a benchmark
BENCHMARK(BM_ThreePlayerPocketPairAVX2);


BENCHMARK_MAIN();
