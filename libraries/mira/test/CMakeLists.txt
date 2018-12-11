file(GLOB UNIT_TESTS "*.cpp")
add_executable( mira_test ${UNIT_TESTS} )
target_link_libraries( mira_test mira fc chainbase ${PLATFORM_SPECIFIC_LIBS} )
