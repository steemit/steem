file(GLOB HEADERS "include/steemit/utilities/*.hpp")

add_library( steemit_plugin_utilities
           )

target_link_libraries( steemit_plugin_utilities fc )
target_include_directories( steemit_plugin_utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_plugin_utilities

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
