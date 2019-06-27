file(GLOB HEADERS "include/dpn/test_api_plugin/*.hpp")
add_library( test_api_plugin
             test_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( test_api_plugin json_rpc_plugin appbase fc )
target_include_directories( test_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   test_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/dpn/test_api_plugin" )
