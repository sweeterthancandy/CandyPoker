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


static void map_lookup(benchmark::State& state, size_t max_domain_value)
{
        constexpr size_t lookup_size = 1'000'00;

        using value_ty = size_t;

        constexpr value_ty max_range_value = 123546789;

        static std::vector<value_ty> lookup_vec;
        static std::vector<value_ty> values_to_lookup;

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



        for (auto _ : state)
        {  

                for (auto key : values_to_lookup)
                {
                        const auto value = lookup_vec[key];
                        benchmark::DoNotOptimize(value);
                }
                
        }  
}

BENCHMARK_CAPTURE(map_lookup, 4, 1ull << 4);
BENCHMARK_CAPTURE(map_lookup, 6, 1ull << 6);
BENCHMARK_CAPTURE(map_lookup, 8, 1ull << 8);
BENCHMARK_CAPTURE(map_lookup, 16, 1ull << 16);
BENCHMARK_CAPTURE(map_lookup, 20, 1ull << 20);
BENCHMARK_CAPTURE(map_lookup, 22, 1ull << 22);
BENCHMARK_CAPTURE(map_lookup, 24, 1ull << 24);
BENCHMARK_CAPTURE(map_lookup, 26, 1ull << 26);
BENCHMARK_CAPTURE(map_lookup, 28, 1ull << 28);

BENCHMARK_MAIN();
