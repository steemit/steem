file( GLOB MIRA_TEST_SOURCES "*.cpp" )
file( GLOB MIRA_TEST_HEADERS "*.hpp" )

add_executable( mira_test ${MIRA_TEST_SOURCES} ${MIRA_TEST_HEADERS} )
target_link_libraries( mira_test mira fc chainbase ${PLATFORM_SPECIFIC_LIBS} )
