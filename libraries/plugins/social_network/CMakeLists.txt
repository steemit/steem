set(CURRENT_TARGET social_network)

list(APPEND CURRENT_TARGET_HEADERS
        include/golos/plugins/social_network/social_network.hpp
        include/golos/plugins/social_network/tag/tags_object.hpp
        include/golos/plugins/social_network/tag/tag_visitor.hpp
        include/golos/plugins/social_network/api_object/category_api_object.hpp
        include/golos/plugins/social_network/api_object/category_index.hpp
        include/golos/plugins/social_network/api_object/comment_api_object.hpp
        include/golos/plugins/social_network/api_object/discussion.hpp
        include/golos/plugins/social_network/api_object/discussion_index.hpp
        include/golos/plugins/social_network/api_object/discussion_query.hpp
        include/golos/plugins/social_network/api_object/tag_api_object.hpp
        include/golos/plugins/social_network/api_object/tag_index.hpp
        include/golos/plugins/social_network/api_object/vote_state.hpp
        include/golos/plugins/social_network/languages/language_object.hpp
        include/golos/plugins/social_network/languages/language_visitor.hpp
        include/golos/plugins/social_network/api_object/account_vote.hpp
)

list(APPEND CURRENT_TARGET_SOURCES
        discussion_query.cpp
        language_visitor.cpp
        social_network.cpp
        tag_visitor.cpp
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
