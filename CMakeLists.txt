cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-std=c++14 ")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

find_package(GTest REQUIRED)

aux_source_directory(test test_sources)

include_directories(.)

add_executable(_ps ps.cpp )
target_link_libraries(_ps)

add_executable(ps_test ${test_sources})
target_link_libraries(ps_test GTest::GTest GTest::Main)

#add_custom_command( TARGET ps_test POST_BUILD COMMAND echo "./ps_test" >> fifo )
add_custom_command( TARGET _ps     POST_BUILD COMMAND echo "./_ps" >> fifo )


