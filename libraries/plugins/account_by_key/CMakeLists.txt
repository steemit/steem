file(GLOB HEADERS "include/steemit/account_by_key/*.hpp")

add_library( steemit_account_by_key
             account_by_key_plugin.cpp
             account_by_key_api.cpp
           )

target_link_libraries( steemit_account_by_key steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_account_by_key
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_account_by_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
