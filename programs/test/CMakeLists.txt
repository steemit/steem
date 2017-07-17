add_executable( appbase_test main.cpp )
target_link_libraries( appbase_test appbase
   webserver_plugin
   test_api_plugin
   json_rpc_plugin
   p2p_plugin
   chain_plugin
   database_api_plugin
   network_broadcast_api_plugin
   witness_plugin
   account_by_key_plugin
   account_by_key_api_plugin
   account_history_plugin
   follow_plugin
   follow_api_plugin
   ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
