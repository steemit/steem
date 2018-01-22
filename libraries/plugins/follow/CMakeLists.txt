set(CURRENT_TARGET follow_plugin)

list(APPEND CURRENT_TARGET_HEADERS
     include/golos/plugins/follow/follow_api_object.hpp
     include/golos/plugins/follow/follow_evaluators.hpp
     include/golos/plugins/follow/follow_objects.hpp
     include/golos/plugins/follow/follow_operations.hpp
     include/golos/plugins/follow/follow_forward.hpp
     include/golos/plugins/follow/plugin.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
     follow_evaluators.cpp
     follow_operations.cpp
     plugin.cpp
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
        golos::json_rpc_plugin
        golos::chain_plugin
        golos::protocol
        golos::social_network
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