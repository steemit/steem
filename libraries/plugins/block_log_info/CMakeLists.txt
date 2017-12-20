file(GLOB HEADERS "include/steem/plugins/block_log_info/*.hpp")

add_library( block_log_info_plugin
             block_log_info_plugin.cpp
           )

target_link_libraries( block_log_info_plugin chain_plugin steem_chain steem_protocol )
target_include_directories( block_log_info_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      block_log_info_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   block_log_info_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
