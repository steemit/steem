file(GLOB HEADERS "include/steemit/plugins/witness/*.hpp")

add_library( witness_plugin
             witness_plugin.cpp
             witness_evaluators.cpp
             witness_operations.cpp
             ${HEADERS}
           )

target_link_libraries( witness_plugin p2p_plugin chain_plugin appbase steemit_chain steemit_utilities steemit_plugin_utilities )
target_include_directories( witness_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   witness_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
