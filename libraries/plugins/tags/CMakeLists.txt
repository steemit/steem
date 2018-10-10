file(GLOB HEADERS "include/steem/plugins/tags/*.hpp")

add_library( tags_plugin
             tags_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( tags_plugin chain_plugin steem_protocol appbase )
target_include_directories( tags_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      tags_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   tags_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
