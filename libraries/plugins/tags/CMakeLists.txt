file(GLOB HEADERS "include/steemit/tags/*.hpp")

add_library(golos_tags
        tags_plugin.cpp)

target_link_libraries(golos_tags golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_tags
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_tags

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
