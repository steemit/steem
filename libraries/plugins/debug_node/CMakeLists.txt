file(GLOB HEADERS "include/steem/plugins/debug_node/*.hpp")

add_library( debug_node_plugin
             ${HEADERS}
             debug_node_plugin.cpp
           )

target_link_libraries( debug_node_plugin chain_plugin steem_utilities witness_plugin )
target_include_directories( debug_node_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      debug_node_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS debug_node_plugin
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
       )
