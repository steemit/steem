file(GLOB COMMON_SOURCES "common/*.cpp")

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling tests with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

if(NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_TEST_DYN_LINK)
endif()

file(GLOB UNIT_TESTS "tests/*.cpp")
add_executable(chain_test ${UNIT_TESTS} ${COMMON_SOURCES})
target_link_libraries(chain_test chainbase golos_chain golos_protocol golos_app golos_account_history golos_market_history golos_debug_node fc ${PLATFORM_SPECIFIC_LIBS})

file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable(plugin_test ${PLUGIN_TESTS} ${COMMON_SOURCES})
target_link_libraries(plugin_test golos_chain golos_protocol golos_app golos_account_history golos_market_history golos_debug_node fc ${PLATFORM_SPECIFIC_LIBS})

if(MSVC)
    set_source_files_properties(tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

#add_subdirectory( generate_empty_blocks )
