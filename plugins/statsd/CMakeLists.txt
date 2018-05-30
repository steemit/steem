set(CURRENT_TARGET statsd)

list(APPEND CURRENT_TARGET_HEADERS
    include/golos/plugins/statsd/plugin.hpp
    include/golos/plugins/statsd/statistics_sender.hpp
    include/golos/plugins/statsd/runtime_bucket_object.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
    plugin.cpp
    statistics_sender.cpp
)

if(BUILD_SHARED_LIBRARIES)
    add_library( golos_${CURRENT_TARGET} SHARED
        ${CURRENT_TARGET_HEADERS}
        ${CURRENT_TARGET_SOURCES}
    )
else()
    add_library( golos_${CURRENT_TARGET} STATIC
        ${CURRENT_TARGET_HEADERS}
        ${CURRENT_TARGET_SOURCES}
    )
endif()

add_library(golos::${CURRENT_TARGET} ALIAS golos_${CURRENT_TARGET})

target_link_libraries(
    golos_${CURRENT_TARGET}
    golos_chain
    golos_chain_plugin
    golos_protocol
    appbase
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

# TODO
# Maybe we should also do smthng like this. 
# install(FILES ${HEADERS} DESTINATION "include/golos/chain_plugin")
