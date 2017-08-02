cmake_minimum_required(VERSION 3.7)

#set(CMAKE_CXX_FLAGS "-std=c++14 -fno-rtti -Wno-switch")
set(CMAKE_CXX_FLAGS "-std=c++14  -Wno-switch")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")



#set(Boost_USE_STATIC_LIBS        ON) # only find static libs
#set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.59.0 REQUIRED COMPONENTS system timer serialization)

include_directories(${Boost_INCLUDE_DIRS})

aux_source_directory(test test_sources)

include_directories(.)


aux_source_directory(lib/base base_sources)
add_library(base_lib SHARED ${base_sources}) 
target_link_libraries(base_lib ${Boost_LIBRARIES})

aux_source_directory(lib/eval base_sources)
add_library(eval_lib SHARED ${base_sources}) 
target_link_libraries(eval_lib ${Boost_LIBRARIES})


add_executable(_ps ps.cpp )
target_link_libraries(_ps pthread )
target_link_libraries(_ps ${Boost_LIBRARIES})
target_link_libraries(_ps base_lib eval_lib )
#target_link_libraries(_ps stdc++fs )

find_package(GTest REQUIRED)
add_executable(ps_test ${test_sources})
target_link_libraries(ps_test GTest::GTest GTest::Main)
target_link_libraries(ps_test base_lib eval_lib )

#add_custom_command( TARGET ps_test POST_BUILD COMMAND echo "./ps_test" >> fifo )
add_custom_command( TARGET _ps     POST_BUILD COMMAND echo "./_ps" >> fifo )

add_executable(pokerstove app/pokerstove.cpp)
target_link_libraries(pokerstove base_lib eval_lib )
target_link_libraries(pokerstove pthread )
target_link_libraries(pokerstove ${Boost_LIBRARIES})

add_executable(create_cache app/create_cache.cpp)
target_link_libraries(create_cache base_lib eval_lib )
target_link_libraries(create_cache pthread )
target_link_libraries(create_cache ${Boost_LIBRARIES})
