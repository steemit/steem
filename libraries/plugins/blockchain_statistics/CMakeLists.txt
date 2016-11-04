file(GLOB HEADERS "include/steemit/blockchain_statistics/*.hpp")

add_library( steemit_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
           )

target_link_libraries( steemit_blockchain_statistics steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)