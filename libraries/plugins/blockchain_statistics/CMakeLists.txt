file(GLOB HEADERS "include/steemit/blockchain_statistics/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_blockchain_statistics SHARED
            blockchain_statistics_plugin.cpp
            blockchain_statistics_api.cpp
            )
else()
    add_library(golos_blockchain_statistics STATIC
            blockchain_statistics_plugin.cpp
            blockchain_statistics_api.cpp
            )
endif()

target_link_libraries(golos_blockchain_statistics golos_chain golos_protocol golos_app)
target_include_directories(golos_blockchain_statistics
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_blockchain_statistics

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )