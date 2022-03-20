#include <benchmark/benchmark.h>

#include "ps/base/frontend.h"
#include "ps/base/tree.h"
#include "ps/interface/interface.h"
#include "ps/eval/pass.h"

using namespace ps;
using namespace ps::interface_;

namespace {
    inline auto do_frontend_parse(std::string const& s)
    {
        return frontend::range(s);
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

    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    for (auto _ : state)
    {
        frontend_to_instruction_list("dummy", { p0, p1 }, false);
    }
}


static void BuildInstrList_3(benchmark::State& state) {

    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    auto p2 = do_frontend_parse("QQ");
    for (auto _ : state)
    {
        
        frontend_to_instruction_list("dummy", { p0, p1, p2 }, false);
    }
}
static void BuildInstrList_4(benchmark::State& state) {

    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    auto p2 = do_frontend_parse("QQ");
    auto p3 = do_frontend_parse("JJ");
    for (auto _ : state)
    {
        
        frontend_to_instruction_list("dummy", { p0, p1, p2, p3 }, false);
    }
}
static void BuildInstrList_5(benchmark::State& state) {
    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    auto p2 = do_frontend_parse("QQ");
    auto p3 = do_frontend_parse("JJ");
    auto p4 = do_frontend_parse("TT");
    for (auto _ : state)
    {
        
        frontend_to_instruction_list("dummy", { p0, p1, p2, p3, p4 }, false);
    }
}
BENCHMARK(BuildInstrList_2);
BENCHMARK(BuildInstrList_3);
BENCHMARK(BuildInstrList_4)->Unit(benchmark::kSecond);
BENCHMARK(BuildInstrList_5)->Unit(benchmark::kSecond);



static void BuildInstrListCache_2(benchmark::State& state) {

    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    for (auto _ : state)
    {
        
        frontend_to_instruction_list("dummy", { p0, p1 }, true);
    }
}
static void BuildInstrListCache_3(benchmark::State& state) {

    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    auto p2 = do_frontend_parse("QQ");
    for (auto _ : state)
    {
        
        frontend_to_instruction_list("dummy", { p0, p1, p2 }, true);
    }
}
static void BuildInstrListCache_4(benchmark::State& state) {
    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    auto p2 = do_frontend_parse("QQ");
    auto p3 = do_frontend_parse("JJ");
    for (auto _ : state)
    {
       
        frontend_to_instruction_list("dummy", { p0, p1, p2, p3 }, true);
    }
}
static void BuildInstrListCache_5(benchmark::State& state) {
    auto p0 = do_frontend_parse("AA");
    auto p1 = do_frontend_parse("KK");
    auto p2 = do_frontend_parse("QQ");
    auto p3 = do_frontend_parse("JJ");
    auto p4 = do_frontend_parse("TT");
    for (auto _ : state)
    {
       
        frontend_to_instruction_list("dummy", { p0, p1, p2, p3, p4 }, true);
    }
}


BENCHMARK(BuildInstrListCache_2);
BENCHMARK(BuildInstrListCache_3);
BENCHMARK(BuildInstrListCache_4)->Unit(benchmark::kSecond);
BENCHMARK(BuildInstrListCache_5)->Unit(benchmark::kSecond);













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
static void Eval_PP_5(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ", "TT"};
        EvaluationObject obj(player_ranges);
        obj.Compute();
    }
}
BENCHMARK(Eval_PP_2)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_3)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_4)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_5)->Unit(benchmark::kSecond);





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
static void Eval_PP_5_Prepare(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ", "TT"};
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
static void Eval_PP_5_After(benchmark::State& state) {
    for (auto _ : state)
    {
        state.PauseTiming();
        std::vector<std::string> player_ranges{ "AA", "KK", "QQ", "JJ", "TT"};
        EvaluationObject obj(player_ranges);
        EvaluationObject prepared = obj.Prepare().get();
        state.ResumeTiming();
        prepared.Compute();
    }
}

BENCHMARK(Eval_PP_2_Prepare)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_2_After)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_3_Prepare)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_3_After)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_4_Prepare)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_4_After)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_5_Prepare)->Unit(benchmark::kSecond);
BENCHMARK(Eval_PP_5_After)->Unit(benchmark::kSecond);






static void Eval_PP_2_Stress(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "22+", "33+"};
        EvaluationObject obj(player_ranges);
        obj.Compute();
    }
}
BENCHMARK(Eval_PP_2_Stress)->Unit(benchmark::kSecond);



static void EvalPrepareStress_100_100(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "100%", "100%"};
        EvaluationObject obj(player_ranges);
        obj.Prepare();
    }
}
BENCHMARK(EvalPrepareStress_100_100)->Unit(benchmark::kSecond);
static void EvalPrepareStressCache_100_100(benchmark::State& state) {
    for (auto _ : state)
    {
        std::vector<std::string> player_ranges{ "100%", "100%"};
        EvaluationObject obj(player_ranges, {}, EvaluationObject::F_CacheInstructions);
        obj.Prepare();
    }
}
BENCHMARK(EvalPrepareStressCache_100_100)->Unit(benchmark::kSecond);



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
