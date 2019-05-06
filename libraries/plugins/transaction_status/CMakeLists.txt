file(GLOB HEADERS "include/steem/plugins/transaction_status/*.hpp")

add_library( transaction_status_plugin
             transaction_status_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( transaction_status_plugin chain_plugin )
target_include_directories( transaction_status_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      transaction_status_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   transaction_status_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
