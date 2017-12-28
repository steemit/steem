file(GLOB HEADERS "include/steemit/account_by_key/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_account_by_key SHARED
            account_by_key_plugin.cpp
            account_by_key_api.cpp
            )
else()
    add_library(golos_account_by_key STATIC
            account_by_key_plugin.cpp
            account_by_key_api.cpp
            )
endif()

target_link_libraries(golos_account_by_key golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_account_by_key
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_account_by_key

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
