file(GLOB HEADERS "include/steemit/account_statistics/*.hpp")

add_library( steemit_account_statistics
             account_statistics_plugin.cpp
             account_statistics_api.cpp
           )

target_link_libraries( steemit_account_statistics steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_account_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_account_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)