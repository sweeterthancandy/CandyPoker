cmake_minimum_required(VERSION 3.14)
project(my_project)
  

add_executable(
  eval-benchmark
  eval_benchmark.cpp

)
target_link_libraries(
  eval-benchmark
)
target_link_libraries(eval-benchmark ${Boost_LIBRARIES})
target_link_libraries(eval-benchmark ps_lib )
target_link_libraries(eval-benchmark benchmark::benchmark )



add_executable(
  iterator-benchmark
  iterator_benchmark.cpp

)
target_link_libraries(
  iterator-benchmark
)
target_link_libraries(iterator-benchmark ${Boost_LIBRARIES})
target_link_libraries(iterator-benchmark ps_lib )
target_link_libraries(iterator-benchmark benchmark::benchmark )
