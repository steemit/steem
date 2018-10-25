file(GLOB HEADERS "include/steem/plugins/rc_api/*.hpp")
add_library( rc_api_plugin
             rc_api_plugin.cpp
             rc_api.cpp
             ${HEADERS}
           )

target_link_libraries( rc_api_plugin rc_plugin json_rpc_plugin )
target_include_directories( rc_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      rc_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   rc_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
