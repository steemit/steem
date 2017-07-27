file(GLOB HEADERS "include/steemit/condenser_api_plugin/*.hpp")
add_library( condenser_api_plugin
             condenser_api.cpp
             condenser_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( condenser_api_plugin
   account_by_key_api_plugin
   account_history_api_plugin
   database_api_plugin
   follow_api_plugin
   market_history_api_plugin
   network_broadcast_api_plugin
   tags_api_plugin
   witness_api_plugin )
target_include_directories( condenser_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   condenser_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steemit/condenser_api_plugin" )
