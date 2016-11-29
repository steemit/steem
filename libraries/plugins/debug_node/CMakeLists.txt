file(GLOB HEADERS "include/steemit/plugins/debug_node/*.hpp")

add_library( steemit_debug_node
             ${HEADERS}
             debug_node_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( steemit_debug_node steemit_app steemit_chain steemit_protocol fc )
target_include_directories( steemit_debug_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS steemit_debug_node
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
       )
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/plugins/debug_node" )
