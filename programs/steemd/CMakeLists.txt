add_executable( steemd main.cpp )
target_link_libraries( steemd appbase steemit_utilities steemit_plugins
   ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
