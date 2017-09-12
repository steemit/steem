file(GLOB HEADERS "include/steemit/plugins/raw_block/*.hpp")

add_library( steem_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( steem_raw_block steem_app steem_chain steem_protocol fc )
target_include_directories( steem_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
