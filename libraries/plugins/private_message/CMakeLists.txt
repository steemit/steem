file(GLOB HEADERS "include/steemit/private_message/*.hpp")

add_library( steemit_private_message
             private_message_plugin.cpp
           )

target_link_libraries( steemit_private_message steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
