cmake_minimum_required(VERSION 3.6)

project(ps)



if( WIN32 )
  #find_package (Eigen3 3.3 REQUIRED NO_MODULE)
  include_directories( C:/work/eigen )
  include_directories( C:/work/eigen/Eigen )



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

  if(0)
          set(CMAKE_CXX_FLAGS "-std=c++14 -frtti -fno-omit-frame-pointer -ggdb3 -mavx -mavx2 -fno-inline ")
  else()
          set(CMAKE_CXX_FLAGS "-std=c++14 -frtti  -Wno-switch")

          set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")
          set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -pedantic")
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
          #for eigen
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-misleading-indentation")
          
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx -mavx2")

          #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  endif()


  #add_definitions(-D_GLIBCXX_ASSERTIONS)

endif()



find_package(Boost REQUIRED COMPONENTS log system timer serialization program_options)

include_directories(${Boost_INCLUDE_DIRS})

include_directories(.)
include_directories( /usr/include/eigen3/ )
include_directories( /usr/include/eigen3/Eigen/ )


#aux_source_directory(lib/base base_sources)
set(base_sources
        lib/base/algorithm.cpp
        lib/base/cards.cpp
        lib/base/frontend.cpp
        lib/base/board_combination_iterator.cpp
        lib/base/tree.cpp
        
        lib/eval/rank_world.cpp
        lib/eval/class_cache.cpp
        lib/eval/holdem_class_vector_cache.cpp
        lib/eval/pass_eval_hand_instr_vec.cpp
        lib/eval/dispatch_generic.cpp
        lib/eval/dispatch_three.cpp
        lib/eval/dispatch_three_avx2.cpp
        lib/eval/dispatch_four.cpp
        
        lib/support/singleton_factory.cpp

        lib/sim/solver.cpp
        lib/sim/solver_numeric.cpp
        lib/sim/solver_permutation.cpp
        #lib/sim/delta_sequence.cpp
        #lib/sim/pipeline.cpp
)
add_library(ps_lib SHARED ${base_sources}) 
target_link_libraries(ps_lib ${Boost_LIBRARIES})
#target_link_libraries(ps_lib Eigen3::Eigen)

                

set( 
        driver_sources

        app/driver.cpp
        #app/cmd_flopzilla.cpp
        app/cmd_solver.cpp
        app/cmd_cache.cpp 
        app/cmd_eval.cpp
)

add_executable(candy-poker ${driver_sources} )


if( WIN32 )
else()
        target_link_libraries(candy-poker pthread )
endif()
target_link_libraries(candy-poker ${Boost_LIBRARIES})
target_link_libraries(candy-poker ps_lib )
#target_link_libraries(candy-poker Eigen3::Eigen)


