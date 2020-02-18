file(GLOB HEADERS "include/steem/plugins/database_api/*.hpp" "include/steem/plugins/database_api/util/*.hpp")

add_library( database_api_plugin
             database_api.cpp
             database_api_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( database_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( database_api_plugin
                            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      database_api_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

INSTALL( TARGETS
   database_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steem/plugins/database_api" )