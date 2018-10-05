file(GLOB HEADERS "include/steem/plugins/block_api/*.hpp")

add_library( block_api_plugin
             block_api.cpp
             block_api_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( block_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( block_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      block_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   block_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
