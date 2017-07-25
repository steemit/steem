file(GLOB HEADERS "include/steemit/plugins/follow_api/*.hpp")
add_library( follow_api_plugin
             follow_api_plugin.cpp
             follow_api.cpp
           )

target_link_libraries( follow_api_plugin follow_plugin database_api_plugin json_rpc_plugin )
target_include_directories( follow_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   follow_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
