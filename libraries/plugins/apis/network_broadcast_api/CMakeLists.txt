file(GLOB HEADERS "include/steem/plugins/network_broadcast_api_plugin/*.hpp")
add_library( network_broadcast_api_plugin
             network_broadcast_api.cpp
             network_broadcast_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( network_broadcast_api_plugin chain_plugin rc_plugin json_rpc_plugin p2p_plugin appbase )
target_include_directories( network_broadcast_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      network_broadcast_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   network_broadcast_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steem/network_broadcast_api_plugin" )
