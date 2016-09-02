file(GLOB HEADERS "include/graphene/time/*.hpp")

add_library( graphene_time
             time.cpp
           )

target_link_libraries( graphene_time fc )
target_include_directories( graphene_time
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_time

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/time" )
