set(CURRENT_TARGET golosd)
add_executable(${CURRENT_TARGET} main.cpp)

if(UNIX AND NOT APPLE)
    set(rt_library rt)
elseif(APPLE)
    list(APPEND PLATFORM_SPECIFIC_LIBS readline)
endif()

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling golosd with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

target_link_libraries(
        ${CURRENT_TARGET} PRIVATE
        appbase
        golos::webserver_plugin
        golos::p2p
        graphene_utilities
        golos::chain_plugin
        golos::network_broadcast_api
        golos::witness
        golos::database_api
        golos::test_api_plugin
        golos::social_network
        golos::market_history
        golos::blockchain_statistics
        golos::account_by_key
        golos::account_history
        golos::private_message
        golos::auth_util
        golos::debug_node
        golos_protocol
        fc
        ${CMAKE_DL_LIBS}
        ${PLATFORM_SPECIFIC_LIBS}
)

install(TARGETS
        ${CURRENT_TARGET}

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
