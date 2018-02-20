file(GLOB HEADERS "include/equihash/*.hpp" )

set( CMAKE_C_FLAGS "-std=c99" )

add_library( equihash
             src/pow.cpp
             src/blake2b.c
           )

target_link_libraries( equihash )
target_include_directories( equihash PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   equihash

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
