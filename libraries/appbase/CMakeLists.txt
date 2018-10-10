# Defines AppBase library target.
project( AppBase )
cmake_minimum_required( VERSION 2.8.12 )

file(GLOB HEADERS "include/appbase/*.hpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread
                             date_time
                             system
                             filesystem
                             chrono
                             program_options
                             unit_test_framework
                             locale)

FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
set( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

if( APPLE )
  # Apple Specific Options Here
  message( STATUS "Configuring AppBase on OS X" )
  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -stdlib=libc++ -Wall -Wno-conversion -Wno-deprecated-declarations" )
else( APPLE )
  # Linux Specific Options Here
  message( STATUS "Configuring AppBase on Linux" )
  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall" )
  set( rt_library rt )
  set( pthread_library pthread)
  if ( FULL_STATIC_BUILD )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
  endif ( FULL_STATIC_BUILD )
endif( APPLE )


if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

add_library( appbase
             application.cpp
             ${HEADERS}
           )

target_link_libraries( appbase ${Boost_LIBRARIES})

target_include_directories( appbase
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${Boost_INCLUDE_DIR})

INSTALL( TARGETS
   appbase

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/appbase" )

add_subdirectory( examples )
