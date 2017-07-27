file(GLOB HEADERS "include/steemit/plugins/debug_node/*.hpp")

add_library( debug_node_plugin
             ${HEADERS}
             debug_node_plugin.cpp
           )

target_link_libraries( debug_node_plugin chain_plugin graphene_utilities )
target_include_directories( debug_node_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS debug_node_plugin
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
       )
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/plugins/debug_node" )
