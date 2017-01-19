file(GLOB HEADERS "include/steemit/plugins/raw_block/*.hpp")

add_library(golos_raw_block
        ${HEADERS}
        raw_block_plugin.cpp
        raw_block_api.cpp
        )

target_link_libraries(golos_raw_block golos_app golos_chain golos_protocol fc)
target_include_directories(golos_raw_block
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
