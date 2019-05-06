file(GLOB HEADERS "include/steem/plugins/account_history_api/*.hpp")
add_library( account_history_api_plugin
             account_history_api_plugin.cpp
             account_history_api.cpp
             ${HEADERS}
           )

target_link_libraries( account_history_api_plugin account_history_plugin account_history_rocksdb_plugin json_rpc_plugin )
target_include_directories( account_history_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      account_history_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   account_history_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
