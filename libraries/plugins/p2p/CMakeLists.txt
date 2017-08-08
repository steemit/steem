file(GLOB HEADERS "include/steemit/plugins/p2p/*.hpp")

add_library( p2p_plugin
             p2p_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( p2p_plugin chain_plugin appbase steemit_chain graphene_net )
target_include_directories( p2p_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   p2p_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
