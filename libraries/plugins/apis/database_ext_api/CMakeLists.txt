file(GLOB HEADERS "include/steemit/plugins/database_ext_api/*.hpp")

add_library( database_ext_api_plugin
             database_ext_api.cpp
             database_ext_api_plugin.cpp
           )

target_link_libraries( database_ext_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( database_ext_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   database_ext_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
