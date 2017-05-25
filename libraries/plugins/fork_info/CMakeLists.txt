file(GLOB HEADERS "include/steemit/fork_info_node/*.hpp")

add_library( steemit_fork_info
             fork_info_plugin.cpp
           )

target_link_libraries( steemit_fork_info steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_fork_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( fork_info_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   steemit_fork_info

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
