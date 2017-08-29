file(GLOB HEADERS "include/steemit/plugins/block_api/*.hpp")

add_library( block_api_plugin
             block_api.cpp
             block_api_plugin.cpp
           )

target_link_libraries( block_api_plugin chain_plugin json_rpc_plugin )
target_include_directories( block_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   block_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
