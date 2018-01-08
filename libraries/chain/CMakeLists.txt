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

             serialized_headers_injected.cpp

             util/reward.cpp
             util/impacted.cpp

             ${HEADERS}
           )

target_link_libraries( steem_chain steem_protocol fc chainbase steem_schema ${PATCH_MERGE_LIB} )
target_include_directories( steem_chain PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../" # for serialize3
                            "${CMAKE_CURRENT_BINARY_DIR}/include" )

#MESSAGE(STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})

#function (ListToString result delim)
#list(GET ARGV 2 temp)
#math(EXPR N "${ARGC}-1")
#foreach(IDX RANGE 3 ${N})
#    list(GET ARGV ${IDX} STR)
#    set(temp "${temp}${delim}${STR}")
#endforeach()
#set(${result} "${temp}" PARENT_SCOPE)
#endfunction(ListToString)

#execute_process(COMMAND /home/syncad/serialize3/build/Release/serialize3/serialize3
#                ./include/steem/chain/serialized_headers.hpp
#                --xml-generator=castxml --xml-generator-path="/home/syncad/castxml/build/Release/bin" --xml-generator-options="-std=c++14 -I../chainbase/include -I./include" --compiler=gcc --compiler-path=/usr/bin --compiler-options="-std=c++14 -I../chainbase/include -I./include"                OUTPUT_VARIABLE RESULT)
#get_target_property(VAR steem_chain INCLUDE_DIRECTORIES)
#SET(STR)
#ListToString(STR " -I" ${VAR})
#SET(STR " -I" ${STR})
#MESSAGE(STATUS "INCLUDE_DIRECTORIES: " ${STR})
#GET_DIRECTORY_PROPERTY(output INCLUDE_DIRECTORIES)
#MESSAGE(WARNING ${output})
                
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
