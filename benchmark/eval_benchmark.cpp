#include <benchmark/benchmark.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"
#include "ps/eval/instruction.h"

using namespace ps;
using namespace ps::interface_;

namespace {
    inline auto do_frontend_parse(std::string const& s)
    {
        return frontend::parse(s);
    }
}



//////////////////////////////////////////////////////////////////////////////////
// benchmark the frontend parser
//////////////////////////////////////////////////////////////////////////////////

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










//////////////////////////////////////////////////////////////////////////////////
// benchmark the frontend parser tree
//////////////////////////////////////////////////////////////////////////////////

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





//////////////////////////////////////////////////////////////////////////////////
// benchmark evaulation preparing
//////////////////////////////////////////////////////////////////////////////////

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






#if 0
static void BM_TwoPlayerPocketPairGenericFirst(benchmark::State& state) {

    state.PauseTiming();
    EvaluationObject::BuildCache();
    state.ResumeTiming();
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        EvaluationObject obj(player_ranges);
        const auto 
    }
}
// Register the function as a benchmark
BENCHMARK(BM_TwoPlayerPocketPairGenericFirst);
#endif


static void BM_AuxMakeCache(benchmark::State& state) {

    
}
//BENCHMARK(BM_AuxMakeCache);





static void BM_TwoPlayerPocketPairGeneric(benchmark::State& state) {
  for (auto _ : state)
  {     
      state.PauseTiming();
      EvaluationObject::BuildCache();
      state.ResumeTiming();

        std::vector<std::string> player_ranges{ "AA", "KK" };
        EvaluationObject obj(player_ranges);
        obj.Compute();
  }
}
static void BM_ThreePlayerPocketPairGeneric(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        EvaluationObject obj(player_ranges);
        obj.Compute();
    }
}
static void BM_FourPlayerPocketPairGeneric(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ"};
        EvaluationObject obj(player_ranges);
        obj.Compute();
    }
}
BENCHMARK(BM_TwoPlayerPocketPairGeneric);
BENCHMARK(BM_ThreePlayerPocketPairGeneric);
BENCHMARK(BM_FourPlayerPocketPairGeneric);






static void BM_TwoPlayerPocketPairGenericPrepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}
static void BM_ThreePlayerPocketPairGenericPrepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}
static void BM_FourPlayerPocketPairGenericPrepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ" };
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}
BENCHMARK(BM_TwoPlayerPocketPairGenericPrepare);
BENCHMARK(BM_ThreePlayerPocketPairGenericPrepare);
BENCHMARK(BM_FourPlayerPocketPairGenericPrepare);









#if 0


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

#endif



BENCHMARK_MAIN();
