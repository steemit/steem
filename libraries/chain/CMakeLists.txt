file(GLOB HEADERS "include/steemit/chain/*.hpp" "include/steemit/chain/util/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( steemit_chain

             # As database takes the longest to compile, start it first
             database.cpp

             smt_evaluator.cpp
             steem_evaluator.cpp
             steem_objects.cpp

             witness_schedule.cpp
             fork_database.cpp

             shared_authority.cpp
             block_log.cpp

             util/reward.cpp
             util/impacted.cpp

             ${HEADERS}
           )

target_link_libraries( steemit_chain steemit_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( steemit_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      steemit_chain PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   steemit_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/chain" )
