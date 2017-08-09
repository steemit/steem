file(GLOB HEADERS "include/steemit/plugins/account_history/*.hpp")

add_library( account_history_plugin
             account_history_plugin.cpp
           )

target_link_libraries( account_history_plugin chain_plugin steemit_chain steemit_protocol )
target_include_directories( account_history_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   account_history_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
