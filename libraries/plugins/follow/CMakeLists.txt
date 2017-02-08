file(GLOB HEADERS "include/steemit/follow/*.hpp")

add_library(golos_follow
        follow_plugin.cpp
        follow_api.cpp
        follow_operations.cpp
        follow_evaluators.cpp
        )

target_link_libraries(golos_follow golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_follow
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_follow

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
