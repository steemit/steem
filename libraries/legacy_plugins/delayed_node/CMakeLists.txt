file(GLOB HEADERS "include/steemit/delayed_node/*.hpp")

add_library( steem_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( steem_delayed_node steem_chain steem_protocol steem_app )
target_include_directories( steem_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   steem_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
