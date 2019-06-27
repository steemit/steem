file(GLOB HEADERS "include/steem/plugins/follow_api/*.hpp")
add_library( follow_api_plugin
             follow_api_plugin.cpp
             follow_api.cpp
             ${HEADERS}
           )

target_link_libraries( follow_api_plugin follow_plugin reputation_api_plugin database_api_plugin json_rpc_plugin )
target_include_directories( follow_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      follow_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   follow_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
