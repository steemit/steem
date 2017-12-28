file(GLOB HEADERS "include/steemit/private_message/*.hpp")

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_private_message SHARED
            private_message_plugin.cpp
            )
else()
    add_library(golos_private_message STATIC
            private_message_plugin.cpp
            )
endif()

target_link_libraries(golos_private_message golos_chain golos_protocol golos_app graphene_time)
target_include_directories(golos_private_message
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_private_message

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
