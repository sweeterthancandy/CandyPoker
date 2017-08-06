cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_FLAGS "-std=c++14 -frtti -Wno-switch")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


#set(Boost_USE_STATIC_LIBS        ON) # only find static libs
#set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.59.0 REQUIRED COMPONENTS system timer serialization)

include_directories(${Boost_INCLUDE_DIRS})

include_directories(.)

#aux_source_directory(lib/base base_sources)
set(base_sources lib/base/cards.cpp)
add_library(base_lib SHARED ${base_sources}) 
target_link_libraries(base_lib ${Boost_LIBRARIES})

set(eval_sources lib/eval/5_card_map.cpp lib/eval/6_card_map.cpp)
add_library(eval_lib SHARED ${eval_sources}) 
target_link_libraries(eval_lib ${Boost_LIBRARIES})


add_executable(_ps ps.cpp )
target_link_libraries(_ps pthread )
target_link_libraries(_ps ${Boost_LIBRARIES})
target_link_libraries(_ps base_lib eval_lib )
#target_link_libraries(_ps stdc++fs )

