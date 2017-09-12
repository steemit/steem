file(GLOB HEADERS "include/steemit/private_message/*.hpp")

add_library( steem_private_message
             private_message_plugin.cpp
           )

target_link_libraries( steem_private_message steem_chain steem_protocol steem_app )
target_include_directories( steem_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steem_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
