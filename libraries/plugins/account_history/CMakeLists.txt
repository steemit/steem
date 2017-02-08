file(GLOB HEADERS "include/steemit/account_history/*.hpp")

add_library(golos_account_history
        account_history_plugin.cpp
        )

target_link_libraries(golos_account_history golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_account_history
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_account_history

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
