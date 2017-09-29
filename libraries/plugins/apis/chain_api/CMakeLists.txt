file( GLOB HEADERS "include/steem/plugins/chain_api/*.hpp" )
set( SOURCES chain_api.cpp chain_api_plugin.cpp )

add_library( chain_api_plugin ${SOURCES} ${HEADERS} )

target_link_libraries( chain_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( chain_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      chain_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   chain_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
