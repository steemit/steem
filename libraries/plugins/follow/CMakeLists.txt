set(CURRENT_TARGET follow_plugin)

list(APPEND CURRENT_TARGET_HEADERS
        include/golos/plugins/follow/follow_api_object.hpp
        include/golos/plugins/follow/follow_api.hpp
        include/golos/plugins/follow/follow_objects.hpp
        include/golos/plugins/follow/follow_operations.hpp
        include/golos/plugins/follow/follow_plugin.hpp

        )

list(APPEND CURRENT_TARGET_SOURCES
        follow_api.cpp
        follow_evaluators.cpp
        follow_operations.cpp
        follow_plugin.cpp
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
        #golos::chain
        golos::json_rpc_plugin
        golos::chain_plugin
        golos::protocol
        golos::database_api_plugin
        appbase
        fc
)
target_include_directories(golos_${CURRENT_TARGET} PUBLIC "include")

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )