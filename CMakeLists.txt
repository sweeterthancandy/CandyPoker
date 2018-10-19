cmake_minimum_required(VERSION 3.6)

find_package (Eigen3 3.3 REQUIRED NO_MODULE)


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
  add_definitions(-DBOOST_LOG_DYN_LINK)
  set(CMAKE_CXX_FLAGS "-std=c++14 -frtti -Wno-switch")

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -pedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")

endif()



find_package(Boost REQUIRED COMPONENTS log system timer serialization)

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
        lib/base/tree.cpp
)
add_library(base_lib SHARED ${base_sources}) 
target_link_libraries(base_lib ${Boost_LIBRARIES})
target_link_libraries(base_lib Eigen3::Eigen)

set(
        eval_sources 
        
        lib/eval/rank_world.cpp
        lib/eval/class_cache.cpp
)
add_library(eval_lib SHARED ${eval_sources}) 
target_link_libraries(eval_lib ${Boost_LIBRARIES})
target_link_libraries(eval_lib Eigen3::Eigen)
                

set( 
        driver_sources

        app/driver.cpp
        app/cmd_heads_up_solver.cpp
)

add_executable(driver ${driver_sources} )
target_link_libraries(driver pthread )
target_link_libraries(driver ${Boost_LIBRARIES})
target_link_libraries(driver support_lib base_lib eval_lib )
target_link_libraries(driver Eigen3::Eigen)


