file(GLOB HEADERS "include/steem/plugins/follow/*.hpp")

add_library( follow_plugin
             follow_plugin.cpp
             follow_operations.cpp
             follow_evaluators.cpp
             inc_performance.cpp
           )

target_link_libraries( follow_plugin chain_plugin )
target_include_directories( follow_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      follow_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   follow_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
