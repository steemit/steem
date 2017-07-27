file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable( chain_test ${UNIT_TESTS} ${COMMON_SOURCES} )
target_link_libraries( chain_test chainbase steemit_chain steemit_protocol account_history_plugin market_history_plugin witness_plugin debug_node_plugin fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable( plugin_test ${PLUGIN_TESTS} ${COMMON_SOURCES} )
target_link_libraries( plugin_test steemit_chain steemit_protocol account_history_plugin market_history_plugin witness_plugin debug_node_plugin fc ${PLATFORM_SPECIFIC_LIBS} )

if(MSVC)
  set_source_files_properties( tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#add_subdirectory( generate_empty_blocks )
