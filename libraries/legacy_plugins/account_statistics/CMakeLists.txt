file(GLOB HEADERS "include/steem/account_statistics/*.hpp")

add_library( steem_account_statistics
             account_statistics_plugin.cpp
             account_statistics_api.cpp
           )

target_link_libraries( steem_account_statistics steem_chain steem_protocol steem_app )
target_include_directories( steem_account_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steem_account_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)