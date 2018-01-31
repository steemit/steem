set(CURRENT_TARGET private_message)

list(APPEND CURRENT_TARGET_HEADERS
        include/golos/plugins/private_message/private_message_plugin.hpp
        include/golos/plugins/private_message/private_message_objects.hpp
        include/golos/plugins/private_message/private_message_evaluators.hpp
        )

list(APPEND CURRENT_TARGET_SOURCES
        private_message_plugin.cpp
        private_message_objects.cpp
        )

if(BUILD_SHARED_LIBRARIES)
    add_library(golos_${CURRENT_TARGET} SHARED
            ${CURRENT_TARGET_HEADERS}
            ${CURRENT_TARGET_SOURCES}
            )
else()
    add_library(golos_${CURRENT_TARGET} STATIC
            ${CURRENT_TARGET_HEADERS}
            ${CURRENT_TARGET_SOURCES}
            )
endif()

add_library(golos::${CURRENT_TARGET} ALIAS golos_${CURRENT_TARGET})
set_property(TARGET golos_${CURRENT_TARGET} PROPERTY EXPORT_NAME ${CURRENT_TARGET})

target_link_libraries(
        golos_${CURRENT_TARGET}
        golos::chain_plugin
        golos::p2p
        golos::protocol
        golos::network
        graphene_utilities
        graphene_time
        appbase
)
target_include_directories(golos_${CURRENT_TARGET}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )

