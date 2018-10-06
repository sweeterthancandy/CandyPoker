cmake_minimum_required(VERSION 3.6)

if( WIN32 )
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  set(BUILD_SHARED_LIBS TRUE)
  
  set(Boost_USE_STATIC_LIBS        ON) # only find static libs
  set(Boost_USE_MULTITHREADED      ON)
  set(Boost_USE_STATIC_RUNTIME    OFF)

  set(BOOST_INCLUDEDIR C:/work/modular-boost )
  set(BOOST_LIBRARYDIR C:/work/modular-boost/stage/lib )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
else()
  set(CMAKE_CXX_FLAGS "-std=c++14 -frtti -Wno-switch")

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")

endif()



find_package(Boost REQUIRED COMPONENTS system timer serialization)

include_directories(${Boost_INCLUDE_DIRS})

include_directories(.)

set(support_sources
        lib/support/singleton_factory.cpp
)
add_library(support_lib SHARED ${support_sources}) 

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
        lib/base/tree.cpp
)
add_library(base_lib SHARED ${base_sources}) 
target_link_libraries(base_lib ${Boost_LIBRARIES})

set(
        eval_sources 
        
        lib/eval/evaluator_5_card_map.cpp
        lib/eval/evaluator_6_card_map.cpp
        lib/eval/rank_world.cpp
        lib/eval/equity_evaluator_principal.cpp 
        lib/eval/equity_evaluator_cached.cpp
        lib/eval/class_equity_evaluator_principal.cpp 
        lib/eval/class_equity_evaluator_proc.cpp 
        lib/eval/class_equity_evaluator_quick.cpp 
        lib/eval/range_equity_evaluator_principal.cpp 
        lib/eval/class_range_equity_evaluator_principal.cpp 
        lib/eval/class_equity_evaluator_cache.cpp
        lib/eval/equity_breakdown.cpp
        lib/eval/holdem_class_eval_cache_impl.cpp
)
add_library(eval_lib SHARED ${eval_sources}) 
target_link_libraries(eval_lib ${Boost_LIBRARIES})


add_executable(_ps ps.cpp )
if(  WIN32 )
else()
  target_link_libraries(_ps pthread )
endif()
target_link_libraries(_ps ${Boost_LIBRARIES})
target_link_libraries(_ps support_lib base_lib eval_lib )

add_executable(pokerstove app/pokerstove.cpp )
target_link_libraries(pokerstove ${Boost_LIBRARIES})
target_link_libraries(pokerstove support_lib base_lib eval_lib )

add_executable(driver app/driver.cpp )
target_link_libraries(driver pthread )
target_link_libraries(driver ${Boost_LIBRARIES})
target_link_libraries(driver support_lib base_lib eval_lib )


#add_executable(heads_up_solver app/heads_up_solver.cpp )
#target_link_libraries(heads_up_solver pthread )
#target_link_libraries(heads_up_solver ${Boost_LIBRARIES})
#target_link_libraries(heads_up_solver base_lib eval_lib )
