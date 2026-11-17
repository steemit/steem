file(GLOB HEADERS "include/steem/plugins/ingest/*.hpp")

add_library( ingest_plugin
             ingest_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( ingest_plugin
                       chain_plugin
                       steem_chain
                       steem_protocol
                       fc
                       ${Boost_LIBRARIES}
                       ${CMAKE_DL_LIBS}
                     )

target_include_directories( ingest_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                          )

if( CLANG_TIDY_EXE )
   set_target_properties(
      ingest_plugin PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

install( TARGETS
   ingest_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
