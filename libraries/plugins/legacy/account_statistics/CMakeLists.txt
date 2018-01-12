file(GLOB HEADERS "include/steemit/account_statistics/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_account_statistics SHARED
            account_statistics_plugin.cpp
            account_statistics_api.cpp
            )
else()
    add_library(golos_account_statistics STATIC
            account_statistics_plugin.cpp
            account_statistics_api.cpp
            )
endif()

target_link_libraries(golos_account_statistics golos_chain golos_protocol golos_app)
target_include_directories(golos_account_statistics
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_account_statistics

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )