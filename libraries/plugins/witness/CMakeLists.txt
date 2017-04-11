file(GLOB HEADERS "include/steemit/witness/*.hpp")

add_library( steemit_witness
             witness_plugin.cpp
             witness_evaluators.cpp
             witness_operations.cpp
           )

target_link_libraries( steemit_witness steemit_chain steemit_protocol steemit_app )
target_include_directories( steemit_witness
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   steemit_witness

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
