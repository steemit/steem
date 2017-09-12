file(GLOB HEADERS "include/steem/plugins/webserver/*.hpp")

add_library( webserver_plugin
             webserver_plugin.cpp
             ${HEADERS} )

target_link_libraries( webserver_plugin json_rpc_plugin chain_plugin appbase fc )
target_include_directories( webserver_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      webserver_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   webserver_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
