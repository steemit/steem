file(GLOB HEADERS "include/steemit/plugin/account_info/*.hpp")

add_library( steemit_account_info
             account_info_api.cpp
             account_info_plugin.cpp
           )

target_link_libraries( steemit_account_info steemit_chain steemit_app graphene_time )
target_include_directories( steemit_account_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_account_info

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
