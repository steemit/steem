file(GLOB HEADERS "include/steem/plugins/block_info/*.hpp")

add_library( steem_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( steem_block_info steem_app steem_chain steem_protocol fc )
target_include_directories( steem_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
