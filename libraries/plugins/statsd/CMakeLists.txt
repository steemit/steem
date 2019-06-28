file(GLOB HEADERS "include/steem/plugins/statsd/*.hpp")

add_library( statsd_plugin
             statsd_plugin.cpp
             utility.cpp
             ${HEADERS} )

target_link_libraries( statsd_plugin chain_plugin )
target_include_directories( statsd_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      statsd_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   statsd_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
