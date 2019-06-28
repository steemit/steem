file(GLOB HEADERS "include/dpn/plugins/raw_block/*.hpp")

add_library( dpn_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( dpn_raw_block dpn_app dpn_chain dpn_protocol fc )
target_include_directories( dpn_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
