file(GLOB HEADERS "include/dpn/delayed_node/*.hpp")

add_library( dpn_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( dpn_delayed_node dpn_chain dpn_protocol dpn_app )
target_include_directories( dpn_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   dpn_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
