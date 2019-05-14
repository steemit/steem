file(GLOB HEADERS "database_fixture.hpp")
set(SOURCES database_fixture.cpp)

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

add_library(db_fixture
            ${SOURCES}
            ${HEADERS})

target_link_libraries(db_fixture
                      steem_chain
                      steem_protocol
                      account_history_plugin
                      market_history_plugin
                      block_api_plugin
                      database_api_plugin
                      rc_plugin
                      webserver_plugin
                      witness_plugin
                      debug_node_plugin
                      condenser_api_plugin
                      fc
                      ${PLATFORM_SPECIFIC_LIBS})
