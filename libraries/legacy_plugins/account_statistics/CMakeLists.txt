file(GLOB HEADERS "include/dpn/account_statistics/*.hpp")

add_library( dpn_account_statistics
             account_statistics_plugin.cpp
             account_statistics_api.cpp
           )

target_link_libraries( dpn_account_statistics dpn_chain dpn_protocol dpn_app )
target_include_directories( dpn_account_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   dpn_account_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)