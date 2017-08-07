add_subdirectory(db_fixture)

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable( chain_test ${UNIT_TESTS} )
target_link_libraries( chain_test db_fixture chainbase steemit_chain steemit_protocol steemit_app steemit_account_history steemit_market_history steemit_witness steemit_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable( plugin_test ${PLUGIN_TESTS} )
target_link_libraries( plugin_test db_fixture steemit_chain steemit_protocol steemit_app steemit_account_history steemit_market_history steemit_witness steemit_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

if(MSVC)
  set_source_files_properties( tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#add_subdirectory( generate_empty_blocks )
