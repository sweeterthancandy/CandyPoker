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

static void RangeParser_Simple(benchmark::State& state) {

    for (auto _ : state)
    {
        do_frontend_parse("AA");
    }
}
static void RangeParser_Complex(benchmark::State& state) {

    for (auto _ : state)
    {
        do_frontend_parse("45s+,68s+,22+,ATo+,AJs+");
    }
}
BENCHMARK(RangeParser_Simple);
BENCHMARK(RangeParser_Complex);










//////////////////////////////////////////////////////////////////////////////////
// benchmark the frontend parser tree
//////////////////////////////////////////////////////////////////////////////////

static void TreeRange_2_Simple(benchmark::State& state) {
    
    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        tree_range root({ p0,p1 });
    }
}

static void TreeRange_3_Simple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        tree_range root({ p0,p1,p2 });
    }
}

static void TreeRange_4_Simple(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        auto p3 = do_frontend_parse("JJ");
        tree_range root({ p0,p1,p2,p3 });
    }
}
BENCHMARK(TreeRange_2_Simple);
BENCHMARK(TreeRange_3_Simple);
BENCHMARK(TreeRange_4_Simple);





//////////////////////////////////////////////////////////////////////////////////
// benchmark evaulation preparing
//////////////////////////////////////////////////////////////////////////////////

static void BuildInstrList_2(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        frontend_to_instruction_list("dummy", { p0, p1 });
    }
}
static void BuildInstrList_3(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        frontend_to_instruction_list("dummy", { p0, p1, p2 });
    }
}
static void BuildInstrList_4(benchmark::State& state) {

    for (auto _ : state)
    {
        auto p0 = do_frontend_parse("AA");
        auto p1 = do_frontend_parse("KK");
        auto p2 = do_frontend_parse("QQ");
        auto p3 = do_frontend_parse("JJ");
        frontend_to_instruction_list("dummy", { p0, p1, p2, p3 });
    }
}
BENCHMARK(BuildInstrList_2);
BENCHMARK(BuildInstrList_3);
BENCHMARK(BuildInstrList_4);










static void Eval_PP_2(benchmark::State& state) {
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
static void Eval_PP_3(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        EvaluationObject obj(player_ranges);
        obj.Compute();
    }
}
static void Eval_PP_4(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ"};
        EvaluationObject obj(player_ranges);
        obj.Compute();
    }
}
BENCHMARK(Eval_PP_2);
BENCHMARK(Eval_PP_3);
BENCHMARK(Eval_PP_4);






static void Eval_PP_2_Prepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK" };
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}
static void Eval_PP_3_Prepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}
static void Eval_PP_4_Prepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ" };
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}








static void Eval_PP_2_After(benchmark::State& state) {
    for (auto _ : state)
    {
        state.PauseTiming(); 
        std::vector<std::string> player_ranges{ "AA", "KK" };
        EvaluationObject obj(player_ranges);
        EvaluationObject prepared = obj.Prepare().get();
        state.ResumeTiming(); 
        prepared.Compute();
    }
}
static void Eval_PP_3_After(benchmark::State& state) {
    for (auto _ : state)
    {
        state.PauseTiming(); 
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ" };
        EvaluationObject obj(player_ranges);
        EvaluationObject prepared = obj.Prepare().get();
        state.ResumeTiming();
        prepared.Compute();
    }
}
static void Eval_PP_4_After(benchmark::State& state) {
    for (auto _ : state)
    {
        state.PauseTiming();
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ" };
        EvaluationObject obj(player_ranges);
        EvaluationObject prepared = obj.Prepare().get();
        state.ResumeTiming();
        prepared.Compute();
    }
}

BENCHMARK(Eval_PP_2_Prepare);
BENCHMARK(Eval_PP_2_After);
BENCHMARK(Eval_PP_3_Prepare);
BENCHMARK(Eval_PP_3_After);
BENCHMARK(Eval_PP_4_Prepare);
BENCHMARK(Eval_PP_4_After);









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
