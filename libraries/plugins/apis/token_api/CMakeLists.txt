file(GLOB HEADERS "include/steem/plugins/token_api/*.hpp")

add_library( token_api_plugin
             token_api.cpp
             token_api_plugin.cpp
	     dict.cpp
           )

target_link_libraries( token_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( token_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      token_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   token_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
