file(GLOB HEADERS "include/steemit/plugins/database_api/*.hpp")

add_library( database_api_plugin
             database_api.cpp
             database_api_plugin.cpp
           )

target_link_libraries( database_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( database_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   database_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
