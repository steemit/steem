file(GLOB HEADERS "include/steem/plugins/transaction_polling_api/*.hpp")

add_library( transaction_polling_api_plugin
             transaction_polling_api.cpp
             transaction_polling_api_plugin.cpp
           )

target_link_libraries( transaction_polling_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( transaction_polling_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      transaction_polling_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   transaction_polling_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
