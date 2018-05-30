set(CURRENT_TARGET tags)

list(APPEND CURRENT_TARGET_HEADERS
        include/golos/plugins/tags/discussion_query.hpp
        include/golos/plugins/tags/plugin.hpp
        include/golos/plugins/tags/tag_api_object.hpp
        include/golos/plugins/tags/tag_visitor.hpp
        include/golos/plugins/tags/tags_object.hpp
        include/golos/plugins/tags/tags_sort.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
        plugin.cpp
        tag_visitor.cpp
        discussion_query.cpp
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
        golos::api
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
