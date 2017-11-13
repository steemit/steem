file(GLOB HEADERS "include/steem/plugins/fork_monitor/*.hpp")

add_library( fork_monitor_plugin
             fork_monitor_plugin.cpp
             ${HEADERS} )
             
target_link_libraries( fork_monitor_plugin chain_plugin )
target_include_directories( fork_monitor_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   fork_monitor_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
