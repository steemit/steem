file(GLOB HEADERS "include/steemit/plugins/webserver/*.hpp")

add_library( webserver_plugin
             webserver_plugin.cpp
             ${HEADERS} )

target_link_libraries( webserver_plugin json_rpc_plugin appbase fc )
target_include_directories( webserver_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   webserver_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
