if(MSVC)
    set(hardfork_hpp_file "${CMAKE_CURRENT_SOURCE_DIR}/include/steemit/chain/hardfork.hpp")
    add_custom_target(build_hardfork_hpp
            COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file})
    add_dependencies(build_hardfork_hpp cat-parts)
else(MSVC)
    set(hardfork_hpp_file "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp")
    add_custom_target(build_hardfork_hpp
            COMMAND "${CMAKE_SOURCE_DIR}/programs/build_helpers/cat_parts.py" "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file})
endif(MSVC)


set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp" PROPERTIES GENERATED TRUE)

## SORT .cpp by most likely to change / break compile
add_library(golos_chain

        # As database takes the longest to compile, start it first
        database.cpp
        fork_database.cpp

        steem_evaluator.cpp

        steem_objects.cpp
        shared_authority.cpp
#        transaction_object.cpp
        block_log.cpp

        include/steemit/chain/account_object.hpp
        include/steemit/chain/block_log.hpp
        include/steemit/chain/block_summary_object.hpp
        include/steemit/chain/comment_object.hpp
        include/steemit/chain/compound.hpp
        include/steemit/chain/custom_operation_interpreter.hpp
        include/steemit/chain/database.hpp
        include/steemit/chain/database_exceptions.hpp
        include/steemit/chain/db_with.hpp
        include/steemit/chain/evaluator.hpp
        include/steemit/chain/evaluator_registry.hpp
        include/steemit/chain/fork_database.hpp
        include/steemit/chain/generic_custom_operation_interpreter.hpp
        include/steemit/chain/global_property_object.hpp
        include/steemit/chain/history_object.hpp
        include/steemit/chain/immutable_chain_parameters.hpp
        include/steemit/chain/index.hpp
        include/steemit/chain/node_property_object.hpp
        include/steemit/chain/operation_notification.hpp
        include/steemit/chain/shared_authority.hpp
        include/steemit/chain/shared_db_merkle.hpp
        include/steemit/chain/snapshot_state.hpp
        include/steemit/chain/steem_evaluator.hpp
        include/steemit/chain/steem_object_types.hpp
        include/steemit/chain/steem_objects.hpp
        include/steemit/chain/transaction_object.hpp
        include/steemit/chain/witness_objects.hpp

        ${hardfork_hpp_file}
        "${CMAKE_CURRENT_BINARY_DIR}/include/steemit/chain/hardfork.hpp"
        )

add_dependencies(golos_chain golos_protocol build_hardfork_hpp)
target_link_libraries(golos_chain golos_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB})
target_include_directories(golos_chain
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include")

if(MSVC)
    set_source_files_properties(database.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif(MSVC)

install(TARGETS
        golos_chain

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
install(FILES ${HEADERS} DESTINATION "include/steemit/chain")
