file(GLOB HEADERS "include/steem/plugins/tags_api/*.hpp")
add_library( tags_api_plugin
             tags_api_plugin.cpp
             tags_api.cpp
           )

target_link_libraries( tags_api_plugin tags_plugin follow_api_plugin json_rpc_plugin )
target_include_directories( tags_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      tags_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   tags_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
