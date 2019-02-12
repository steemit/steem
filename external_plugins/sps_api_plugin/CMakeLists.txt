file(GLOB HEADERS "include/steem/plugins/sps_api_plugin/*.hpp")

add_library( sps_api_plugin
             ${HEADERS}
             sps_api_plugin.cpp
           )

target_link_libraries( sps_api_plugin appbase steem_chain fc )
target_include_directories( sps_api_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
