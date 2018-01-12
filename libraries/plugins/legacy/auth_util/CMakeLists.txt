file(GLOB HEADERS "include/steemit/plugins/auth_util/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_auth_util SHARED
            ${HEADERS}
            auth_util_plugin.cpp
            auth_util_api.cpp
            )
else()
    add_library(golos_auth_util STATIC
            ${HEADERS}
            auth_util_plugin.cpp
            auth_util_api.cpp
            )
endif()

target_link_libraries(golos_auth_util golos_app golos_chain golos_protocol fc)
target_include_directories(golos_auth_util
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
