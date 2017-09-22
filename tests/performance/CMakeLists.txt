file(GLOB HEADERS "data.hpp")
set(SOURCES data.cpp)

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

add_library(performance_data
            ${SOURCES}
            ${HEADERS})

target_link_libraries(performance_data
                      steem_chain
                      steem_protocol
                      account_history_plugin
                      market_history_plugin
                      witness_plugin
                      debug_node_plugin
                      fc
                      ${PLATFORM_SPECIFIC_LIBS})
