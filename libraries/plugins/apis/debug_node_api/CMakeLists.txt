file(GLOB HEADERS "include/steemit/plugins/debug_node_api/*.hpp")
add_library( debug_node_api_plugin
             debug_node_api_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( debug_node_api_plugin debug_node_plugin json_rpc_plugin database_api_plugin )
target_include_directories( debug_node_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   debug_node_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
