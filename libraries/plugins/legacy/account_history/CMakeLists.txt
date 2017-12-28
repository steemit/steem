file(GLOB HEADERS "include/steemit/account_history/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_account_history SHARED
            account_history_plugin.cpp
            )
else()
    add_library(golos_account_history STATIC
            account_history_plugin.cpp
            )
endif()

target_link_libraries(golos_account_history golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_account_history
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_account_history

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
