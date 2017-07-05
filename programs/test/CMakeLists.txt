add_executable( appbase_test main.cpp )
target_link_libraries( appbase_test appbase http_plugin test_api_plugin api_register_plugin p2p_plugin chain_plugin ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
