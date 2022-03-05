#include <benchmark/benchmark.h>

#include <vector>
#include <array>
#include <random>
#include <iostream>
#include <cmath>




static void map_lookup(benchmark::State& state)
{

        

        for (auto _ : state)
        {  

                state.PauseTiming();
                
                constexpr size_t lookup_size = 1'000'000;

                using value_ty = int;

                constexpr size_t lookup_bits = 10;

                constexpr value_ty max_domain_value = 1 << lookup_bits;
                constexpr value_ty max_range_value = 123546789;


                std::random_device seeder;
                std::mt19937 engine(seeder());

                std::uniform_int_distribution<value_ty> domain_dist(0, max_domain_value);
                std::uniform_int_distribution<value_ty> range_dist(0, max_range_value);

        
                std::vector<value_ty> lookup_vec;
                lookup_vec.resize(max_domain_value);
                constexpr size_t num_non_zero = 100;
                for(size_t idx=0;idx!=num_non_zero;++idx)
                {
                        lookup_vec[domain_dist(engine)] = range_dist(engine);
                }

                std::vector<value_ty> values_to_lookup;
                values_to_lookup.resize(lookup_size);
                for(size_t idx=0;idx!=lookup_size;++idx)
                {
                        values_to_lookup[idx] = domain_dist(engine);
                }

                state.ResumeTiming();


                for (auto key : values_to_lookup)
                {
                        const auto value = lookup_vec[key];
                        benchmark::DoNotOptimize(value);
                }
                
        }  
}

BENCHMARK(map_lookup)->Unit(benchmark::kMillisecond);


BENCHMARK_MAIN();
