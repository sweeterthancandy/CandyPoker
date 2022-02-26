cmake_minimum_required(VERSION 3.14)
project(my_project)
  

add_executable(
  ps-test
  eval.cpp
  cards.cpp
  frontend.cpp

)
target_link_libraries(
  ps-test
  gtest_main
)
target_link_libraries(ps-test ${Boost_LIBRARIES})
target_link_libraries(ps-test ps_lib )
