file(GLOB HEADERS "include/steem/plugins/witness_api/*.hpp")
add_library( witness_api_plugin
             witness_api_plugin.cpp
             witness_api.cpp
           )

target_link_libraries( witness_api_plugin witness_plugin json_rpc_plugin )
target_include_directories( witness_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      witness_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   witness_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
