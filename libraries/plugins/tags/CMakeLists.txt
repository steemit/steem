file(GLOB HEADERS "include/steemit/plugins/tags/*.hpp")

add_library( tags_plugin
             tags_plugin.cpp )

target_link_libraries( tags_plugin chain_plugin steemit_protocol appbase )
target_include_directories( tags_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   tags_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
