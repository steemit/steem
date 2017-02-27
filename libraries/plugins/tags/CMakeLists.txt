file(GLOB HEADERS "include/steemit/tags/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_tags SHARED
            tags_plugin.cpp)
else()
    add_library(golos_tags STATIC
            tags_plugin.cpp)
endif()

target_link_libraries(golos_tags golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_tags
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_tags

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
