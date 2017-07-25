file(GLOB HEADERS "include/steemit/plugins/follow/*.hpp")

add_library( follow_plugin
             follow_plugin.cpp
             follow_operations.cpp
             follow_evaluators.cpp
           )

target_link_libraries( follow_plugin chain_plugin )
target_include_directories( follow_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   follow_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
