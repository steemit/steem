file(GLOB HEADERS "include/steemit/delayed_node/*.hpp")

add_library( steemit_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( steemit_delayed_node steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   steemit_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
