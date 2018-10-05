file(GLOB HEADERS "include/steem/plugins/reputation/*.hpp")

add_library( reputation_plugin
             reputation_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( reputation_plugin chain_plugin )
target_include_directories( reputation_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      reputation_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   reputation_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
