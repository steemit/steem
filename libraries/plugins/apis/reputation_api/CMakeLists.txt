file(GLOB HEADERS "include/steem/plugins/reputation_api/*.hpp")
add_library( reputation_api_plugin
             reputation_api_plugin.cpp
             reputation_api.cpp
             ${HEADERS}
           )

target_link_libraries( reputation_api_plugin reputation_plugin database_api_plugin json_rpc_plugin )
target_include_directories( reputation_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      reputation_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   reputation_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
