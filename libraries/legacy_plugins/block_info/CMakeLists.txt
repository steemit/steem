file(GLOB HEADERS "include/dpn/plugins/block_info/*.hpp")

add_library( dpn_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( dpn_block_info dpn_app dpn_chain dpn_protocol fc )
target_include_directories( dpn_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
