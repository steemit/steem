file(GLOB HEADERS "include/steemit/condenser_api_plugin/*.hpp")
add_library( condenser_api_plugin
             condenser_api.cpp
             condenser_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( condenser_api_plugin chain_plugin json_rpc_plugin steemit_app appbase )
target_include_directories( condenser_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   condenser_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steemit/condenser_api_plugin" )
