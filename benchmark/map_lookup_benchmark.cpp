#include <benchmark/benchmark.h>

#include <vector>
#include <array>
#include <random>
#include <iostream>
#include <cmath>
#include <thread>

#if 0
static void sleep_test(benchmark::State& state)
{
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for(auto _ : state);
}
BENCHMARK(sleep_test);
#endif

using value_ty = size_t;

struct MapLookupAux
{
        static std::vector<value_ty> lookup_vec;
        static std::vector<value_ty> values_to_lookup;

        explicit MapLookupAux(size_t max_domain_value)
        {
                constexpr size_t lookup_size = 1'000'00;
                constexpr value_ty max_range_value = 123546789;

        

                constexpr size_t vec_size = 1ull << 28;
                if (lookup_vec.empty())
                {
                        lookup_vec.resize(vec_size);
                        values_to_lookup.resize(lookup_size);
                }

                std::random_device seeder;
                std::mt19937 engine(seeder());

                std::uniform_int_distribution<value_ty> domain_dist(0, static_cast<value_ty>(max_domain_value));
                std::uniform_int_distribution<value_ty> range_dist(0, max_range_value);

        
        
                        
                // this shouldn't really matter
                constexpr size_t num_non_zero = 10000;
                for(size_t idx=0;idx!=num_non_zero;++idx)
                {
                        lookup_vec[domain_dist(engine)] = range_dist(engine);
                }
                        
                for(size_t idx=0;idx!=lookup_size;++idx)
                {
                        values_to_lookup[idx] = domain_dist(engine);
                }
        }
};

std::vector<value_ty> MapLookupAux::lookup_vec;
std::vector<value_ty> MapLookupAux::values_to_lookup;

static void MapLookupUnconditional(benchmark::State& state)
{
        
        const size_t max_domain_value = state.range(0);
        MapLookupAux aux(max_domain_value);

        for (auto _ : state)
        {  
                for (auto key : aux.values_to_lookup)
                {
                        const auto value = aux.lookup_vec[key];
                        benchmark::DoNotOptimize(value);
                }
        }  
}
BENCHMARK(MapLookupUnconditional)->RangeMultiplier(2)->Range(1ull << 8, 1ull << 28);


static void MapLookupConditionalUnlikelyLookup(benchmark::State& state)
{
        
        const size_t max_domain_value = state.range(0);
        MapLookupAux aux(max_domain_value);

        for (auto _ : state)
        {  
                for (auto key : aux.values_to_lookup)
                {
                        if( ( key % 27 ) != 0 )
                                continue;
                        const auto value = aux.lookup_vec[key];
                        benchmark::DoNotOptimize(value);
                }
        }  
}
BENCHMARK(MapLookupConditionalUnlikelyLookup)->RangeMultiplier(2)->Range(1ull << 8, 1ull << 28);

static void MapLookupConditionalLikelyLookup(benchmark::State& state)
{
        
        const size_t max_domain_value = state.range(0);
        MapLookupAux aux(max_domain_value);

        for (auto _ : state)
        {  
                for (auto key : aux.values_to_lookup)
                {
                        if( ( key % 27 ) == 0 )
                                continue;
                        const auto value = aux.lookup_vec[key];
                        benchmark::DoNotOptimize(value);
                }
        }  
}
BENCHMARK(MapLookupConditionalLikelyLookup)->RangeMultiplier(2)->Range(1ull << 8, 1ull << 28);

BENCHMARK_MAIN();
