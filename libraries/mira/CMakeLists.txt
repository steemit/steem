# Defines MIRA library target.
project( MIRA )
cmake_minimum_required( VERSION 2.8.12 )

file(GLOB HEADERS "include/mira/*.hpp" "include/mira/detail/*.hpp")

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread
                             date_time
                             system
                             filesystem
                             chrono
                             unit_test_framework
                             locale)

FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
set( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

if( APPLE )
  # Apple Specific Options Here
  message( STATUS "Configuring MIRA on OS X" )
  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -stdlib=libc++ -Wall" )
else( APPLE )
  # Linux Specific Options Here
  message( STATUS "Configuring MIRA on Linux" )
  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall" )
  set( rt_library rt )
  set( pthread_library pthread)
  if ( FULL_STATIC_BUILD )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
  endif ( FULL_STATIC_BUILD )
endif( APPLE )

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
endif()

if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

add_library( mira
             src/mira.cpp
             src/configuration.cpp
             ${HEADERS}
           )

if( CLANG_TIDY_EXE )
   set_target_properties(
      mira PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

target_link_libraries( mira ${Boost_LIBRARIES} rocksdb fc )

target_include_directories( mira
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
          "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/rocksdb/include"
          ${Boost_INCLUDE_DIR}
   )

add_subdirectory( test )

INSTALL( TARGETS
   mira

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/mira" )
