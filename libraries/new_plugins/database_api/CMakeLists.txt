file(GLOB HEADERS "include/steemit/database_api_plugin/*.hpp")
add_library( database_api_plugin
             database_api.cpp
             database_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( database_api_plugin chain_plugin api_register_plugin steemit_app appbase )
target_include_directories( database_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   database_api_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/steemit/database_api_plugin" )
