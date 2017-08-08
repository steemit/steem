file(GLOB HEADERS "include/steemit/plugins/json_rpc/*.hpp")

add_library( json_rpc_plugin
             json_rpc_plugin.cpp
             ${HEADERS} )

target_link_libraries( json_rpc_plugin appbase fc )
target_include_directories( json_rpc_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   json_rpc_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
