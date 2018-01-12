file(GLOB HEADERS "include/steemit/plugins/block_info/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_block_info SHARED
            ${HEADERS}
            block_info_plugin.cpp
            block_info_api.cpp
            )
else()
    add_library(golos_block_info STATIC
            ${HEADERS}
            block_info_plugin.cpp
            block_info_api.cpp
            )
endif()

target_link_libraries(golos_block_info golos_app golos_chain golos_protocol fc)
target_include_directories(golos_block_info
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
