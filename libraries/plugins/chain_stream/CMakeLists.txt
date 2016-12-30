file(GLOB HEADERS "include/steemit/plugins/chain_stream/*.hpp")

add_library( steemit_chain_stream
             ${HEADERS}
             chain_stream_plugin.cpp
             chain_stream_api.cpp
           )

target_link_libraries( steemit_chain_stream steemit_app steemit_chain steemit_protocol )
target_include_directories( steemit_chain_stream
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
