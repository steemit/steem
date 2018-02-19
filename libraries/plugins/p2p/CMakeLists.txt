file(GLOB HEADERS "include/steem/plugins/p2p/*.hpp")

add_library( p2p_plugin
             p2p_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( p2p_plugin chain_plugin appbase steem_chain graphene_net )
target_include_directories( p2p_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      p2p_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   p2p_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
