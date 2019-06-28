file(GLOB HEADERS "include/dpn/blockchain_statistics/*.hpp")

add_library( dpn_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
           )

target_link_libraries( dpn_blockchain_statistics dpn_chain dpn_protocol dpn_app )
target_include_directories( dpn_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   dpn_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)