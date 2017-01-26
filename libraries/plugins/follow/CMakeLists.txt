file(GLOB HEADERS "include/steemit/follow/*.hpp")

add_library( steemit_follow
             follow_plugin.cpp
             follow_api.cpp
             follow_operations.cpp
             follow_evaluators.cpp
           )

target_link_libraries( steemit_follow steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_follow
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_follow

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
