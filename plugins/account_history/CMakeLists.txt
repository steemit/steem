set(CURRENT_TARGET account_history)

list(APPEND CURRENT_TARGET_HEADERS
    include/golos/plugins/account_history/plugin.hpp
    include/golos/plugins/account_history/history_object.hpp
    include/golos/plugins/account_history/applied_operation.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
    plugin.cpp
    applied_operation.cpp
)

if (BUILD_SHARED_LIBRARIES)
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

target_link_libraries (
    golos_${CURRENT_TARGET}
    golos_chain
    golos_chain_plugin
    golos_protocol
    appbase
    golos_json_rpc
    graphene_time
    chainbase
    fc
)

target_include_directories(golos_${CURRENT_TARGET}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../"
)

install(TARGETS
    golos_${CURRENT_TARGET}

    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)