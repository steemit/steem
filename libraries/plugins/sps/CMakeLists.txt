file(GLOB HEADERS "include/steem/plugins/sps/*.hpp")

add_library( sps_plugin
             sps_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( sps_plugin steem_protocol chain_plugin )
target_include_directories( sps_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      sps_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   sps_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
