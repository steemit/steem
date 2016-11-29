file(GLOB HEADERS "include/steemit/plugins/block_info/*.hpp")

add_library( steemit_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( steemit_block_info steemit_app steemit_chain steemit_protocol fc )
target_include_directories( steemit_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
