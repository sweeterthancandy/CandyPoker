cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_FLAGS "-std=c++14 -frtti -Wno-switch")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")


#set(Boost_USE_STATIC_LIBS        ON) # only find static libs
#set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.59.0 REQUIRED COMPONENTS system timer serialization)

include_directories(${Boost_INCLUDE_DIRS})

include_directories(.)

#aux_source_directory(lib/base base_sources)
set(base_sources
        lib/base/algorithm.cpp
        lib/base/cards.cpp
        lib/base/frontend.cpp
        lib/base/board_combination_iterator.cpp
        lib/base/holdem_hand_vector.cpp
        lib/base/holdem_class_vector.cpp
        lib/base/holdem_class_range_vector.cpp
        lib/base/card_vector.cpp
        lib/base/holdem_class_range.cpp
        lib/base/rank_vector.cpp
)
add_library(base_lib SHARED ${base_sources}) 
target_link_libraries(base_lib ${Boost_LIBRARIES})

set(
        eval_sources 
        
        lib/eval/5_card_map.cpp
        lib/eval/6_card_map.cpp
        lib/eval/rank_world.cpp
        lib/eval/equity_evaluator_principal.cpp 
        lib/eval/equity_evaluator_cached.cpp
        lib/eval/class_equity_evaluator_principal.cpp 
        lib/eval/class_equity_evaluator_proc.cpp 
        lib/eval/range_equity_evaluator_principal.cpp 
        lib/eval/class_range_equity_evaluator_principal.cpp 
        lib/eval/class_equity_evaluator_cache.cpp
        lib/eval/equity_breakdown.cpp
)
add_library(eval_lib SHARED ${eval_sources}) 
target_link_libraries(eval_lib ${Boost_LIBRARIES})
target_link_libraries(eval_lib base_lib)


add_executable(_ps ps.cpp )
target_link_libraries(_ps pthread )
target_link_libraries(_ps ${Boost_LIBRARIES})
target_link_libraries(_ps base_lib eval_lib )
#target_link_libraries(_ps stdc++fs )

