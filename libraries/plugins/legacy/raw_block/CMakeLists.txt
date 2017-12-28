file(GLOB HEADERS "include/steemit/plugins/raw_block/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_raw_block SHARED
            ${HEADERS}
            raw_block_plugin.cpp
            raw_block_api.cpp
            )
else()
    add_library(golos_raw_block STATIC
            ${HEADERS}
            raw_block_plugin.cpp
            raw_block_api.cpp
            )
endif()

target_link_libraries(golos_raw_block golos_app golos_chain golos_protocol fc)
target_include_directories(golos_raw_block
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
