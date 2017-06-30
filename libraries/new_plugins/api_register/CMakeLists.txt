file(GLOB HEADERS "include/steemit/plugins/api_register_plugin/*.hpp")
add_library( api_register_plugin
             api_register_plugin.cpp
             ${HEADERS} )

target_link_libraries( api_register_plugin appbase fc )
target_include_directories( api_register_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   api_register_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steemit/api_register_plugin" )
