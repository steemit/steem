file(GLOB HEADERS "include/dpn/plugins/smt_test/*.hpp")

add_library( smt_test_plugin
             smt_test_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( smt_test_plugin chain_plugin dpn_chain dpn_protocol )
target_include_directories( smt_test_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   smt_test_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
