
# for each subdirectory containing a CMakeLists.txt, add that subdirectory

file( GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} * )
set( ENV{STEEMIT_EXTERNAL_PLUGINS} "" )
set( ENV{STEEMIT_INTERNAL_PLUGINS} "" )
foreach( child ${children} )
   if( IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${child}" )
      if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${child}/CMakeLists.txt" )
         add_subdirectory( "${child}" )
         set( ENV{STEEMIT_INTERNAL_PLUGINS} "$ENV{STEEMIT_INTERNAL_PLUGINS};${child}" )
      endif()
   endif()
endforeach()
