file(GLOB HEADERS "include/steem/plugins/stats_export/*.hpp")

add_library( stats_export_plugin
             stats_export_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( stats_export_plugin block_data_export_plugin chain_plugin steem_chain steem_protocol )
target_include_directories( stats_export_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      stats_export_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   stats_export_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
