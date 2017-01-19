file(GLOB HEADERS "include/steemit/plugins/block_info/*.hpp")

add_library(golos_block_info
        ${HEADERS}
        block_info_plugin.cpp
        block_info_api.cpp
        )

target_link_libraries(golos_block_info golos_app golos_chain golos_protocol fc)
target_include_directories(golos_block_info
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
