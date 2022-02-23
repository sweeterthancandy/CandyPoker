cmake_minimum_required(VERSION 3.14)
project(my_project)
  

add_executable(
  hello_test
  hello_test.cc
)
target_link_libraries(
  hello_test
  gtest_main
)
target_link_libraries(hello_test ${Boost_LIBRARIES})
target_link_libraries(hello_test ps_lib )
