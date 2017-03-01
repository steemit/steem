file(GLOB HEADERS "include/steemit/time/*.hpp")

add_library( steemit_time
             time.cpp
           )

target_link_libraries( steemit_time fc )
target_include_directories( steemit_time
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_time

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steemit/time" )
