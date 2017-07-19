file(GLOB HEADERS "include/steemit/plugins/account_by_key/*.hpp")

add_library( account_by_key_plugin
             account_by_key_plugin.cpp
           )

target_link_libraries( account_by_key_plugin chain_plugin steemit_chain steemit_protocol )
target_include_directories( account_by_key_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   account_by_key_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
