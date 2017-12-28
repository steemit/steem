file(GLOB HEADERS "include/steemit/follow/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_follow SHARED
            follow_plugin.cpp
            follow_api.cpp
            follow_operations.cpp
            follow_evaluators.cpp
            )
else()
    add_library(golos_follow STATIC
            follow_plugin.cpp
            follow_api.cpp
            follow_operations.cpp
            follow_evaluators.cpp
            )
endif()

target_link_libraries(golos_follow golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_follow
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_follow

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
