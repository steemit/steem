file(GLOB HEADERS "include/steemit/account_statistics/*.hpp")

add_library(golos_account_statistics
        account_statistics_plugin.cpp
        account_statistics_api.cpp
        )

target_link_libraries(golos_account_statistics golos_chain golos_protocol golos_app)
target_include_directories(golos_account_statistics
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_account_statistics

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )