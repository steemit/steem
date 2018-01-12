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

             ${CMAKE_CURRENT_BINARY_DIR}/serialized_headers_injected.cpp

             util/reward.cpp
             util/impacted.cpp

             ${HEADERS}
           )

add_dependencies( steem_chain castxml serialize3 )
target_link_libraries( steem_chain steem_protocol fc chainbase steem_schema ${PATCH_MERGE_LIB} )
target_include_directories( steem_chain PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            #"${serialize3_SOURCE_DIR}/../"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../../programs"
                            "${CMAKE_CURRENT_BINARY_DIR}/include" )

#get_property(INCLUDES TARGET steem_chain PROPERTY INCLUDE_DIRECTORIES)
SET(INCLUDES "-I$<JOIN:$<TARGET_PROPERTY:steem_chain,INCLUDE_DIRECTORIES>, -I>")
MESSAGE(STATUS "INCLUDES: ${INCLUDES}")
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/serialized_headers_injected.cpp
   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/include/steem/chain/serialized_headers.hpp
   DEPENDS castxml serialize3
   IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/include/steem/chain/serialized_headers.hpp
   USES_TERMINAL 
   COMMENT  "RUN SERIALIZE3 ON ${CMAKE_CURRENT_SOURCE_DIR}/include/steem/chain/serialized_headers.hpp"
   COMMAND  ${serialize3_BINARY_DIR}/serialize3
            ${CMAKE_CURRENT_SOURCE_DIR}/include/steem/chain/serialized_headers.hpp
            -d ${CMAKE_CURRENT_BINARY_DIR}
            --xml-generator castxml
            --xml-generator-path ${CastXML_BINARY_DIR}/bin
            --xml-generator-options "-undef -std=c++14"
            --compiler ${CMAKE_CXX_COMPILER}
            --compiler-options "${CMAKE_CXX_FLAGS} ${INCLUDES} -D_X86INTRIN_H_INCLUDED"
            #--compiler-includes ${INCLUDES}
            --check-changes preprocess
            --indent 3
            #--verbose
   )
                                                     
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
