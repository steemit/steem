file(GLOB HEADERS "include/steemit/account_history/*.hpp")

add_library( steemit_account_history
             account_history_plugin.cpp
           )

target_link_libraries( steemit_account_history steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
