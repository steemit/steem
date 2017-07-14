file(GLOB HEADERS "include/steemit/plugins/network_broadcast_api_plugin/*.hpp")
add_library( network_broadcast_api_plugin
             network_broadcast_api.cpp
             network_broadcast_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( network_broadcast_api_plugin chain_plugin json_rpc_plugin p2p_plugin appbase )
target_include_directories( network_broadcast_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   network_broadcast_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steemit/network_broadcast_api_plugin" )
