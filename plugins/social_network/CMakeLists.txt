set(CURRENT_TARGET social_network)

list(APPEND CURRENT_TARGET_HEADERS
        include/golos/plugins/social_network/social_network.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
        social_network.cpp
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
        golos_chain
        golos::chain_plugin
        golos::network
        golos::follow
        golos::tags
        appbase
)

target_include_directories(
        golos_${CURRENT_TARGET}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../"
)

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
