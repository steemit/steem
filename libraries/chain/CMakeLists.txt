file(GLOB HEADERS "include/steemit/chain/*.hpp")

add_custom_target( build_hardfork_hpp
                   COMMAND cat-parts hardfork.d "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp" )
set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

add_dependencies( build_hardfork_hpp cat-parts )

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/object_dirs.txt" "${CMAKE_CURRENT_SOURCE_DIR}/objects")

foreach(object_dir $ENV{STEEMIT_EXTERNAL_PLUGIN_OBJECT_DIRS})
   file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/object_dirs.txt" "${object_dir}\n")
endforeach()

foreach(object_dir $ENV{STEEMIT_INTERNAL_PLUGIN_OBJECT_DIRS})
   file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/object_dirs.txt" "${object_dir}\n")
endforeach()

add_custom_target( build_objects
                   COMMAND object_builder
                   -o "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/object"
                   -l "${CMAKE_CURRENT_BINARY_DIR}/object_dirs.txt"
                   -t "${CMAKE_CURRENT_SOURCE_DIR}/object_hpp.template"
                 )

## SORT .cpp by most likely to change / break compile
add_library( steemit_chain

             # As database takes the longest to compile, start it first
             database.cpp
             fork_database.cpp

             protocol/types.cpp
             protocol/authority.cpp
             protocol/operations.cpp
             protocol/steem_operations.cpp
             protocol/transaction.cpp
             protocol/block.cpp
             protocol/asset.cpp

             get_config.cpp

             evaluator.cpp
             steem_evaluator.cpp

#             account_object.cpp
             steem_objects.cpp
             block_database.cpp

             ${HEADERS}
             "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp"
           )

add_dependencies( steemit_chain build_hardfork_hpp build_objects )
target_link_libraries( steemit_chain fc graphene_db ${PATCH_MERGE_LIB} )
target_include_directories( steemit_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp block_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   steemit_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
