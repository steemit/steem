add_executable( appbase_test main.cpp )
target_link_libraries( appbase_test appbase steemit_plugins
   ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
