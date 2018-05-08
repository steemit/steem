set(CURRENT_TARGET api)

list(APPEND CURRENT_TARGET_HEADERS
    include/golos/api/extended_account.hpp
    include/golos/api/account_api_object.hpp
    include/golos/api/comment_api_object.hpp
    include/golos/api/discussion.hpp
    include/golos/api/vote_state.hpp
    include/golos/api/account_vote.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
    account_api_object.cpp
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
    golos_protocol
    graphene_utilities
    fc
)

target_include_directories(golos_${CURRENT_TARGET}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(TARGETS
        golos_${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )