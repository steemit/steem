file(GLOB HEADERS "include/steem/plugins/account_by_key/*.hpp")

add_library( account_by_key_plugin
             account_by_key_plugin.cpp
           )

target_link_libraries( account_by_key_plugin chain_plugin steem_chain steem_protocol )
target_include_directories( account_by_key_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      account_by_key_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   account_by_key_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
