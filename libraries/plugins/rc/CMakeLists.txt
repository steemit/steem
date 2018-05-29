file(GLOB HEADERS "include/steem/plugins/rc/*.hpp")

add_library( rc_plugin
             rc_plugin.cpp
             rc_utility.cpp
             resource_count.cpp
             resource_user.cpp
           )

target_link_libraries( rc_plugin chain_plugin steem_chain steem_jsonball steem_protocol )
target_include_directories( rc_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      rc_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   rc_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
