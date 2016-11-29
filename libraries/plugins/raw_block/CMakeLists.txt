file(GLOB HEADERS "include/steemit/plugins/raw_block/*.hpp")

add_library( steemit_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( steemit_raw_block steemit_app steemit_chain steemit_protocol fc )
target_include_directories( steemit_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
