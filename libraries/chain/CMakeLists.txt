file(GLOB HEADERS "include/steem/chain/*.hpp" "include/steem/chain/util/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( steem_chain

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

target_link_libraries( steem_chain steem_protocol fc chainbase steem_schema ${PATCH_MERGE_LIB} )

FIND_LIBRARY(PROFILER_LIB profiler PATHS "${CMAKE_CURRENT_SOURCE_DIR}/steem-gperftools/lib")

if (PROFILER_LIB)
   MESSAGE(STATUS "Google perftools profiler library found: ${PROFILER_LIB}")
   target_link_libraries( steem_chain ${PROFILER_LIB} )
else()
   MESSAGE(STATUS "Google perftools not installed - profiler library not found")
endif()

target_include_directories( steem_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if( CLANG_TIDY_EXE )
   set_target_properties(
      steem_chain PROPERTIES
      CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
   )
endif( CLANG_TIDY_EXE )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   steem_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steem/chain" )
