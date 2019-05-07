file(GLOB UNIT_TEST_SOURCES "*.cpp")
file(GLOB UNIT_TEST_HEADERS "*.hpp")
add_executable( mira_test ${UNIT_TEST_SOURCES} ${UNIT_TEST_HEADERS} )
target_link_libraries( mira_test mira fc chainbase steem_protocol steem_utilities ${PLATFORM_SPECIFIC_LIBS} )
