file(GLOB COMMON_SOURCES
        common/database_fixture.cpp
        common/database_fixture.hpp
        common/comment_reward.hpp)

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
target_include_directories(chain_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/common")
target_link_libraries(
        chain_test
        chainbase
        golos_chain
        golos_protocol
        golos_account_history
        golos_market_history
        golos_debug_node
        fc ${PLATFORM_SPECIFIC_LIBS})

add_test(NAME chain_test_run COMMAND chain_test)

file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
add_executable(plugin_test ${PLUGIN_TESTS} ${COMMON_SOURCES})
target_link_libraries(plugin_test golos_chain golos_protocol  golos_account_history golos_market_history golos_debug_node fc ${PLATFORM_SPECIFIC_LIBS})
target_include_directories(plugin_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/common")
add_test(NAME plugin_test_run COMMAND plugin_test)

if(MSVC)
    set_source_files_properties(tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

# add_subdirectory( generate_empty_blocks )
