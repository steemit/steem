file(GLOB HEADERS "include/steem/plugins/market_history_api/*.hpp")
add_library( market_history_api_plugin
             market_history_api_plugin.cpp
             market_history_api.cpp
             ${HEADERS}
           )

target_link_libraries( market_history_api_plugin market_history_plugin json_rpc_plugin )
target_include_directories( market_history_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      market_history_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   market_history_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
