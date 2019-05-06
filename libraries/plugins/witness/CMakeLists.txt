file(GLOB HEADERS "include/steem/plugins/witness/*.hpp")

add_library( witness_plugin
             witness_plugin.cpp
             block_producer.cpp
             ${HEADERS}
           )

target_link_libraries( witness_plugin p2p_plugin chain_plugin rc_plugin appbase steem_chain steem_utilities steem_utilities )
target_include_directories( witness_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      witness_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   witness_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
