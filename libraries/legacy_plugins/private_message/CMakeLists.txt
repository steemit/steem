file(GLOB HEADERS "include/dpn/private_message/*.hpp")

add_library( dpn_private_message
             private_message_plugin.cpp
           )

target_link_libraries( dpn_private_message dpn_chain dpn_protocol dpn_app )
target_include_directories( dpn_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   dpn_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
