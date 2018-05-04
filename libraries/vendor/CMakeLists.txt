# for each subdirectory containing a CMakeLists.txt, add that subdirectory

set(BUILD_SHARED_LIBS OFF)

#Configure needed build options of RocksDB.
SET(WITH_TESTS OFF CACHE BOOL "build with tests")
SET(WITH_SNAPPY ON CACHE BOOL "build with SNAPPY")
SET(WITH_ZLIB ON CACHE BOOL "build with ZLIB")
SET(WITH_BZ2 ON CACHE BOOL "build with BZ2")
SET(WITH_BENCHMARKS OFF CACHE BOOL "build with BENCHMARKS")

file( GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} * )
foreach( child ${children} )
   if( IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${child}" )
      if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt" )
         add_subdirectory( "${child}" )
      endif()
   endif()
endforeach()

