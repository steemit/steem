file(GLOB HEADERS "include/steem/plugins/sps_api/*.hpp")

add_library( sps_api_plugin
             sps_api_plugin.cpp
             sps_api.cpp
             ${HEADERS}
           )

target_link_libraries( sps_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( sps_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      sps_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   sps_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)